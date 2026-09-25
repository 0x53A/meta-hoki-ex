#include "LocationAPI.h"
#include <poll.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <time.h>
#include <stddef.h>
#include <string.h>

static_assert(sizeof(LocationCallbacks)==368,"callback ABI");
static_assert(sizeof(std::function<void()>)==24,"libc++ ABI");
static_assert(sizeof(TrackingOptions)==24,"tracking ABI");
static_assert(sizeof(LocationControlCallbacks)==80 && offsetof(LocationControlCallbacks,responseCb)==8 && offsetof(LocationControlCallbacks,collectiveResponseCb)==32 && offsetof(LocationControlCallbacks,gnssConfigCb)==56,"control callback ABI");
static_assert(sizeof(Location)==72,"location ABI");
static_assert(offsetof(Location,timestamp)==8 && offsetof(Location,latitude)==16 && offsetof(Location,accuracy)==48,"location fields");
static pthread_mutex_t mu=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv=PTHREAD_COND_INITIALIZER;
static volatile sig_atomic_t interrupted=0;
static unsigned caps_count=0,fix_count=0,valid_count=0,seq=0;
static bool destroyed=false;
struct Response {unsigned sequence,id; int error;};
static Response replies[64];
static void signal_handler(int sig){
 if(sig==SIGALRM)_exit(128+sig);
 if(!interrupted)alarm(30);interrupted=1; // Hard process exit if a vendor call or cleanup never returns.
}
static void capabilities(LocationCapabilitiesMask mask){
 pthread_mutex_lock(&mu);++caps_count;printf("CAPABILITIES mask=0x%x\n",mask);pthread_cond_broadcast(&cv);pthread_mutex_unlock(&mu);
}
static void response(LocationError err,uint32_t id){
 pthread_mutex_lock(&mu);++seq;replies[seq%64]={seq,id,int(err)};printf("RESPONSE seq=%u id=%u error=%d\n",seq,id,int(err));pthread_cond_broadcast(&cv);pthread_mutex_unlock(&mu);
}
static void collective(size_t count,LocationError*,uint32_t*){printf("COLLECTIVE count=%zu\n",count);}
static void tracking(Location loc){
 if(loc.size!=sizeof(Location) || !(loc.flags&1) || !loc.timestamp || !__builtin_isfinite(loc.latitude) || !__builtin_isfinite(loc.longitude) || loc.latitude < -90 || loc.latitude > 90 || loc.longitude < -180 || loc.longitude > 180)return;
 flockfile(stdout);
 printf("HOKI1 POSITION %u %llu %.17g %.17g %.17g %.9g %.9g %.9g %.9g\n",loc.flags,(unsigned long long)loc.timestamp,loc.latitude,loc.longitude,loc.altitude,loc.speed,loc.bearing,loc.accuracy,loc.verticalAccuracy);
 funlockfile(stdout);
}
static_assert(sizeof(GnssSv)==36 && offsetof(GnssSvNotification,gnssSvs)==12 && offsetof(GnssSv,gnssSvOptionsMask)==24,"satellite ABI");
static void satellites(GnssSvNotification n){
 if(n.size!=sizeof(n) || n.count>GNSS_SV_MAX){fprintf(stderr,"Satellite ABI mismatch\n");return;}
 for(unsigned i=0;i<n.count;i++)if(n.gnssSvs[i].size!=sizeof(GnssSv))return;
 flockfile(stdout);printf("HOKI1 SV %zu",n.count);
 for(unsigned i=0;i<n.count;i++){const auto &v=n.gnssSvs[i];printf(" %u %u %.9g %.9g %.9g %u",v.svId,unsigned(v.type),v.cN0Dbhz,v.elevation,v.azimuth,v.gnssSvOptionsMask);}
 puts("");funlockfile(stdout);
}
static_assert(sizeof(GnssNmeaNotification)==24 && offsetof(GnssNmeaNotification,nmea)==16 && offsetof(GnssNmeaNotification,length)==20,"NMEA ABI");
static void nmea(GnssNmeaNotification n){ /* GNSS-specific registration selects GNSS over FLP; no text retained. */ (void)n; }
static void complete(){pthread_mutex_lock(&mu);destroyed=true;puts("DESTROY_COMPLETE");pthread_cond_broadcast(&cv);pthread_mutex_unlock(&mu);}
static long long now_ms(){timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (long long)t.tv_sec*1000+t.tv_nsec/1000000;}
static void tick(){timespec t;clock_gettime(CLOCK_REALTIME,&t);++t.tv_sec;pthread_cond_timedwait(&cv,&mu,&t);}
static unsigned checkpoint(){pthread_mutex_lock(&mu);unsigned n=seq;pthread_mutex_unlock(&mu);return n;}
static bool wait_response(unsigned id,unsigned after,int seconds,bool cancellable=true){
 long long end=now_ms()+seconds*1000;bool ok=false;pthread_mutex_lock(&mu);
 while(now_ms()<end){
  for(auto &r:replies)if(r.sequence>after && r.id==id){ok=r.error==0;goto done;}
  if(cancellable && interrupted)break;tick();
 }
 done:pthread_mutex_unlock(&mu);return ok;
}
static bool check_vtable(LocationAPI* api,void* handle){
 struct Entry{unsigned slot;const char* symbol;};
 Entry es[]={{2,"_ZN11LocationAPI15updateCallbacksER17LocationCallbacks"},{3,"_ZN11LocationAPI13startTrackingER15TrackingOptions"},{4,"_ZN11LocationAPI12stopTrackingEj"},{5,"_ZN11LocationAPI21updateTrackingOptionsEjR15TrackingOptions"}};
 void** vt=*reinterpret_cast<void***>(api);
 for(auto &e:es){void* expected=dlsym(handle,e.symbol);if(!expected || vt[e.slot]!=expected){printf("VTABLE_MISMATCH slot=%u symbol=%s\n",e.slot,e.symbol);return false;}}
 puts("VTABLE_OK selected_tracking_slots=4");return true;
}
// Private pipe protocol: argv interval-ms, stdin UPDATE <ms> or STOP; EOF stops.
int main(int argc,char** argv){
 setvbuf(stdout,nullptr,_IOLBF,0);
 if(argc!=2)return 2;
 char *end=nullptr;unsigned long interval=strtoul(argv[1],&end,10);
 if(!end||*end||interval<1000||interval>3600000)return 2;
 int owner=open("/run/user/1000/hoki-location.lock",O_CREAT|O_CLOEXEC|O_RDWR,0600);
 if(owner<0 || flock(owner,LOCK_EX|LOCK_NB))return 3;
 signal(SIGINT,signal_handler);signal(SIGTERM,signal_handler);signal(SIGALRM,signal_handler);signal(SIGPIPE,SIG_IGN);
 prctl(PR_SET_PDEATHSIG,SIGTERM);if(getppid()==1)return 3;
 alarm(45);
 void *handle=dlopen("liblocation_api.so",RTLD_NOW|RTLD_LOCAL);if(!handle)return 3;
 LocationCallbacks callbacks={};callbacks.size=sizeof(callbacks);callbacks.capabilitiesCb=capabilities;callbacks.responseCb=response;callbacks.collectiveResponseCb=collective;callbacks.trackingCb=tracking;callbacks.gnssSvCb=satellites;
 auto *api=LocationAPI::createInstance(callbacks);if(!api)return 4;
 bool success=check_vtable(api,handle);
 LocationControlCallbacks cc={};cc.size=sizeof(cc);cc.responseCb=response;cc.collectiveResponseCb=collective;
 auto *control=LocationControlAPI::createInstance(cc);unsigned controlId=0,id=0;
 if(control && success){unsigned mark=checkpoint();controlId=control->enable(LOCATION_TECHNOLOGY_TYPE_GNSS);success=wait_response(controlId,mark,10);}else success=false;
 TrackingOptions opts={};opts.size=sizeof(opts);opts.minInterval=interval;opts.mode=GNSS_SUPL_MODE_STANDALONE;opts.powerMode=GNSS_POWER_MODE_INVALID;
 if(success && !interrupted){unsigned mark=checkpoint();id=api->LocationAPI::startTracking(opts);success=wait_response(id,mark,10);}
 if(success && !interrupted){puts("HOKI1 READY");alarm(0);}
 char input[128];unsigned used=0;
 while(success && !interrupted){
  pollfd fd={STDIN_FILENO,POLLIN,0};int rc=poll(&fd,1,-1);if(rc<0){if(errno==EINTR)continue;break;}if(!rc)continue;
  char ch;int n=read(STDIN_FILENO,&ch,1);if(n<=0)break;
  if(ch!='\n'){if(used>=sizeof(input)-1){success=false;break;}input[used++]=ch;continue;}
  input[used]=0;used=0;if(!strcmp(input,"STOP"))break;
  if(strncmp(input,"UPDATE ",7)){success=false;break;}
  unsigned long next=strtoul(input+7,&end,10);if(*end||next<1000||next>3600000){success=false;break;}
  alarm(30);opts.minInterval=next;unsigned mark=checkpoint();api->LocationAPI::updateTrackingOptions(id,opts);success=wait_response(id,mark,10);
  if(success){printf("HOKI1 UPDATED %lu\n",next);alarm(0);}
 }
 alarm(30);
 if(id){unsigned mark=checkpoint();api->LocationAPI::stopTracking(id);bool ok=wait_response(id,mark,10,false);printf("HOKI1 STOP_ACK %d\n",ok);success=success&&ok;}
 if(control){if(controlId){unsigned mark=checkpoint();control->disable(controlId);bool ok=wait_response(controlId,mark,10,false);printf("HOKI1 DISABLE_ACK %d\n",ok);success=success&&ok;}control->destroy();}
 api->destroy(complete);long long deadline=now_ms()+10000;pthread_mutex_lock(&mu);while(!destroyed && now_ms()<deadline)tick();bool done=destroyed;pthread_mutex_unlock(&mu);
 printf("HOKI1 EXIT %d\n",success&&done);fflush(stdout);_exit(success&&done?0:5);
}

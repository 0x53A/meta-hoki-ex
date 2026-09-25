/* Hoki QMI LOC assistance probe. Android/bionic helper calling vendor QCCI directly.
 * Default: read-only orbit source/validity queries. Optional injection needs an
 * active GPS session. No engine control, NV, calibration or firmware writes.
 * Protocol: AOSP marlin location_service_v02.{h,c}; libqmi LOC JSON.
 * Every mutation checks both QMI response and asynchronous LOC indication.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <sys/prctl.h>
#include <signal.h>
#define hybris_dlopen dlopen
#define hybris_dlsym dlsym
#define hybris_dlerror dlerror
typedef void (*ind_fn)(void *, unsigned, void *, unsigned, void *);
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static unsigned wanted, got, ilen;
static unsigned char ibuf[8192], replybuf[8192];
static unsigned replylen;
static void dump(const char *tag,unsigned id,const unsigned char *p,unsigned n) {
 flockfile(stdout);
 printf("%s id=0x%04x len=%u",tag,id,n);
 for(unsigned i=0;i<n;i++) printf(" %02x",p[i]);
 puts(""); fflush(stdout); funlockfile(stdout);
}
static void indication(void *c,unsigned id,void *p,unsigned n,void *d) {
 (void)c;(void)d; pthread_mutex_lock(&lock);
 dump("IND",id,p,n);
 if(id==wanted && !got && n<=sizeof ibuf) {memcpy(ibuf,p,n);ilen=n;got=1;}
 pthread_mutex_unlock(&lock);
}
static int (*sendraw)(void *,unsigned,void *,unsigned,void *,unsigned,unsigned *,unsigned);
static uint32_t le(const unsigned char *p,unsigned n) {
 uint32_t v=0;for(unsigned i=0;i<n;i++)v|=(uint32_t)p[i]<<(8*i);return v;
}
static const unsigned char *tlv(const unsigned char *p,unsigned n,unsigned type,unsigned size) {
 for(unsigned i=0;i+3<=n;){unsigned len=le(p+i+1,2);if(i+3+len>n)return 0;
 if(p[i]==type && len==size)return p+i+3;
 i+=3+len;}return 0;
}
static unsigned add(unsigned char *p,unsigned type,uint64_t value,unsigned len) {
 p[0]=type;p[1]=len;p[2]=len>>8;for(unsigned i=0;i<len;i++)p[3+i]=value>>(8*i);return 3+len;
}
static int request(void *client,unsigned id,void *buf,unsigned n) {
 unsigned char resp[8192];unsigned len=0;
 pthread_mutex_lock(&lock);wanted=id;got=0;ilen=0;pthread_mutex_unlock(&lock);
 int rc=sendraw(client,id,buf,n,resp,sizeof resp,&len,5000);
 printf("SEND id=0x%04x transport=%d\n",id,rc);
 if(rc) return rc;
 dump("RSP",id,resp,len);
 const unsigned char *result=tlv(resp,len,2,4);
 if(!result||le(result,2)){fprintf(stderr,"QMI result failed\n");return 11;}
 struct timespec delay={0,100000000};
 for(int i=0;i<100;i++) {pthread_mutex_lock(&lock);int done=got;if(done){memcpy(replybuf,ibuf,ilen);replylen=ilen;wanted=0;}pthread_mutex_unlock(&lock);if(done){const unsigned char *status=tlv(replybuf,replylen,1,4);
 if(!status)return 12;
 unsigned code=le(status,4);printf("LOC_STATUS id=0x%04x status=%u\n",id,code);return code?20+code:0;}nanosleep(&delay,0);}
 fprintf(stderr,"Indication timeout for %04x\n",id);return 10;
}
int main(int argc,char **argv) {
 if(!(argc==1 || (argc==2 && !strcmp(argv[1],"--inject-time")) ||
      (argc==3 && !strcmp(argv[1],"--inject-xtra")))) {
 fprintf(stderr,"Usage: %s [--inject-time | --inject-xtra FILE]\n",argv[0]);return 64;
 }
 prctl(PR_SET_PDEATHSIG,SIGTERM);if(getppid()==1)return 3;
 setvbuf(stdout,0,_IOLBF,0);
 void *lib=hybris_dlopen("/vendor/lib/libloc_api_v02.so",2);
 if(!lib){fprintf(stderr,"loc dlopen: %s\n",hybris_dlerror());return 1;}
 void *svc=hybris_dlsym(lib,"loc_qmi_idl_service_object_v02");
 void *qmi=hybris_dlopen("/vendor/lib/libqmi_cci.so",2);
 if(!qmi||!svc){fprintf(stderr,"symbols: %s\n",hybris_dlerror());return 2;}
 int (*init)(void *,unsigned,ind_fn,void *,void *,unsigned,void **)=hybris_dlsym(qmi,"qmi_client_init_instance");
 int (*release)(void *)=hybris_dlsym(qmi,"qmi_client_release");
 sendraw=hybris_dlsym(qmi,"qmi_client_send_raw_msg_sync");
 if(!init||!release||!sendraw)return 3;
 void *client=0;int rc=init(svc,0xffff,indication,0,0,5000,&client);
 printf("INIT result=%d client=%p\n",rc,client);if(rc)return 4;
 int a=request(client,0x36,0,0),b=request(client,0x37,0,0);
 if(argc>=2 && strcmp(argv[1],"--inject-time")==0) {
 struct timespec now;clock_gettime(CLOCK_REALTIME,&now);unsigned char p[32];
 unsigned n=add(p,1,(uint64_t)now.tv_sec*1000+now.tv_nsec/1000000,8);
 n+=add(p+n,2,1000,4);a=request(client,0x38,p,n);
 b=0; /* Orbit validity is independent of whether UTC injection was accepted. */
 } else if(argc==3 && strcmp(argv[1],"--inject-xtra")==0) {
 FILE *f=fopen(argv[2],"rb");if(!f){perror("xtra");release(client);return 30;}
 fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);
 if(size<=0||size>1048576){fclose(f);release(client);return 31;}
 unsigned parts=(size+1023)/1024;
 for(unsigned part=1;part<=parts;part++) {
 unsigned char p[1100];unsigned n=add(p,1,size,4);n+=add(p+n,2,parts,2);n+=add(p+n,3,part,2);
 unsigned count=(size-(part-1)*1024)>1024?1024:size-(part-1)*1024;
 p[n++]=4;p[n++]=(count+2)&255;p[n++]=(count+2)>>8;p[n++]=count&255;p[n++]=count>>8;
 if(fread(p+n,1,count,f)!=count){a=32;break;}n+=count;n+=add(p+n,0x10,0,4);
 printf("XTRA part=%u/%u bytes=%u\n",part,parts,count);a=request(client,0xa7,p,n);
 if(a)break;
 const unsigned char *ack=tlv(replybuf,replylen,0x10,2);
 if(!ack||le(ack,2)!=part){a=33;fprintf(stderr,"Wrong/missing part ACK\n");break;}
 if(part==parts){const unsigned char *mask=tlv(replybuf,replylen,0x11,8);if(!mask||!le(mask,4)){a=34;fprintf(stderr,"No accepted constellation mask\n");break;}printf("XTRA_ACCEPTED mask=0x%x\n",le(mask,4));}
 }
 fclose(f);if(!a){b=request(client,0x37,0,0);
 if(!b){const unsigned char *v=tlv(replybuf,replylen,0x10,10);if(!v || !le(v+8,2))b=35;else {uint64_t start=(uint64_t)le(v,4)|((uint64_t)le(v+4,4)<<32);uint64_t until=start+(uint64_t)le(v+8,2)*3600;time_t now=time(0);printf("VALIDITY start=%llu end=%llu\n",(unsigned long long)start,(unsigned long long)until);if(now<0 || (uint64_t)now<start || (uint64_t)now>=until)b=36;}}
 }
 }
 printf("RELEASE result=%d\n",release(client));return a||b;
}

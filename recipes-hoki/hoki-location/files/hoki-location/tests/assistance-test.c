/* SPDX-License-Identifier: MIT */
#define dlopen fake_dlopen
#define dlsym fake_dlsym
#define main assistance_main
#include "../assistance.c"
#undef main
#undef dlopen
#undef dlsym
static ind_fn callback;
static int injected;
static int fake_init(void *s,unsigned i,ind_fn cb,void *d,void *o,unsigned t,void **c) {
 (void)s;(void)i;(void)d;(void)o;(void)t;callback=cb;*c=(void*)1;return 0;
}
static int fake_release(void *c) {(void)c;return 0;}
static int fake_send(void *c,unsigned id,void *p,unsigned n,void *out,unsigned cap,unsigned *len,unsigned timeout) {
 (void)p;(void)n;(void)cap;(void)timeout;
 if(id==0x37)return 77; /* Orbit-validity query fails independently of UTC. */
 unsigned char result[]={2,4,0,0,0,0,0};memcpy(out,result,sizeof result);*len=sizeof result;
 unsigned char status[]={1,4,0,0,0,0,0};callback(c,id,status,sizeof status,0);
 if(id==0x38)++injected;
 return 0;
}
void *fake_dlopen(const char *path,int flags) {(void)path;(void)flags;return (void*)1;}
void *fake_dlsym(void *h,const char *name) {
 (void)h;
 if(!strcmp(name,"loc_qmi_idl_service_object_v02"))return (void*)1;
 if(!strcmp(name,"qmi_client_init_instance"))return (void*)fake_init;
 if(!strcmp(name,"qmi_client_release"))return (void*)fake_release;
 if(!strcmp(name,"qmi_client_send_raw_msg_sync"))return (void*)fake_send;
 return 0;
}
int main(void) {
 char *args[]={"assistance-test","--inject-time",0};
 int rc=assistance_main(2,args);
 if(rc || injected!=1){fprintf(stderr,"FAIL: accepted UTC incorrectly tied to orbit validity\n");return 1;}
 puts("PASS: accepted UTC independent of failing orbit query");return 0;
}

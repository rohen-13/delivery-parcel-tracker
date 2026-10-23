#define _POSIX_C_SOURCE 200809L
#include "round_store.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool Token(FILE *f,char token[128])
{
    if(fscanf(f,"%127s",token)!=1)return false;
    int c=fgetc(f);
    return c==EOF?!ferror(f):isspace((unsigned char)c)!=0;
}

static bool UInt(FILE *f,unsigned max,unsigned *value)
{
    char token[128],*end;
    if(!Token(f,token))return false;
    for(char *p=token;*p;++p)if(!isdigit((unsigned char)*p))return false;
    errno=0;unsigned long n=strtoul(token,&end,10);
    if(errno || *end || n>max)return false;
    *value=(unsigned)n;return true;
}

static bool Number(FILE *f,double min,double max,double *value)
{
    char token[128],*end;
    if(!Token(f,token))return false;
    errno=0;double n=strtod(token,&end);
    if(errno || end==token || *end || !isfinite(n) || n<min || n>max)return false;
    *value=n;return true;
}

static bool ReadRound(FILE *f,DeliveryRound *r)
{
    char magic[128];unsigned version,selected;
    if(!Token(f,magic) || strcmp(magic,"PARCEL_ROUND") || !UInt(f,1,&version) || version!=1)return false;
    RoundReset(r);
    if(!UInt(f,DISPATCH_MAX_PACKAGES-1,&selected) || !Number(f,-85,85,&r->position.lat) || !Number(f,-180,180,&r->position.lon))return false;
    r->selected=(int)selected;
    for(int i=0;i<DISPATCH_MAX_PACKAGES;++i) {
        unsigned included,delivered,deferred,heating,connected;
        Sensors *s=&r->sensors[i];TemperatureHistory *h=&r->histories[i];
        double temp,peak;
        if(!UInt(f,1,&included) || !UInt(f,1,&delivered) || !UInt(f,1,&deferred) ||
           !UInt(f,UINT_MAX,&r->attempt_count[i]) || (delivered && (included||deferred)) || (deferred&&included))return false;
        r->packages[i].included=included;r->packages[i].delivered=delivered;r->deferred[i]=deferred;
        if(!Number(f,20,40,&temp) || !UInt(f,1,&heating) || !UInt(f,UINT_MAX,&s->impact_count) ||
           !UInt(f,1,&connected) || !Number(f,0,1e12,&s->seconds) ||
           !Number(f,0,s->seconds,&s->stale_seconds) || !Number(f,temp,40,&peak) ||
           !Number(f,0,s->seconds,&s->heat_seconds) || !Number(f,0,s->seconds,&s->unobserved_seconds) ||
           !UInt(f,SENSOR_EVENT_CAPACITY,&s->event_count))return false;
        s->temperature_c=(float)temp;s->peak_temperature_c=(float)peak;s->heating=heating;s->connected=connected;
        if((connected&&s->stale_seconds!=0) || s->stale_seconds>s->unobserved_seconds)return false;
        double previous=0;
        for(unsigned j=0;j<s->event_count;++j) {
            SensorEvent *e=&s->events[j];unsigned kind;
            if(!Number(f,previous,s->seconds,&e->seconds) || !UInt(f,SENSOR_RECONNECTED,&kind) ||
               !Number(f,20,40,&temp) || !UInt(f,s->impact_count,&e->impact_count))return false;
            e->kind=(SensorEventKind)kind;e->temperature_c=(float)temp;previous=e->seconds;
        }
        if(!UInt(f,TEMPERATURE_HISTORY_CAPACITY,&h->count))return false;
        previous=0;
        for(unsigned j=0;j<h->count;++j) {
            TemperatureSample *sample=&h->samples[j];unsigned fresh;
            if(!Number(f,previous,s->seconds,&sample->seconds) || (j&&sample->seconds==previous) ||
               !Number(f,20,40,&temp) || !UInt(f,1,&fresh))return false;
            sample->temperature_c=(float)temp;sample->fresh=fresh;previous=sample->seconds;
        }
        unsigned count=r->attempt_count[i];
        unsigned first=count>ROUND_RETAINED_ATTEMPTS?count-ROUND_RETAINED_ATTEMPTS:0;
        previous=0;
        for(unsigned j=first;j<count;++j) {
            DeliveryAttempt *a=&r->attempts[i][j%ROUND_RETAINED_ATTEMPTS];unsigned outcome;
            if(!Number(f,previous,s->seconds,&a->seconds) || !UInt(f,ATTEMPT_ADDRESS_PROBLEM,&outcome))return false;
            if(outcome==ATTEMPT_DELIVERED && (j!=count-1 || !delivered))return false;
            a->outcome=(AttemptOutcome)outcome;previous=a->seconds;
        }
        if((delivered||deferred) && !count)return false;
        if(count) {
            AttemptOutcome last=r->attempts[i][(count-1)%ROUND_RETAINED_ATTEMPTS].outcome;
            if(delivered!=(last==ATTEMPT_DELIVERED))return false;
        }
    }
    int c;do{c=fgetc(f);}while(c!=EOF && isspace((unsigned char)c));
    if(c!=EOF || ferror(f))return false;
    snprintf(r->route.message,sizeof(r->route.message),"Round resumed. Plan a fresh driving route.");
    return true;
}

static bool WriteRound(const DeliveryRound *r,FILE *f)
{
    fprintf(f,"PARCEL_ROUND 1\n%d %a %a\n",r->selected,r->position.lat,r->position.lon);
    for(int i=0;i<DISPATCH_MAX_PACKAGES;++i) {
        const Sensors *s=&r->sensors[i];const TemperatureHistory *h=&r->histories[i];
        if(s->event_count>SENSOR_EVENT_CAPACITY || h->count>TEMPERATURE_HISTORY_CAPACITY)return false;
        fprintf(f,"%u %u %u %u\n",r->packages[i].included,r->packages[i].delivered,r->deferred[i],r->attempt_count[i]);
        fprintf(f,"%a %u %u %u %a %a %a %a %a %u\n",(double)s->temperature_c,s->heating,s->impact_count,s->connected,
                s->seconds,s->stale_seconds,(double)s->peak_temperature_c,s->heat_seconds,s->unobserved_seconds,s->event_count);
        for(unsigned j=0;j<s->event_count;++j) {
            const SensorEvent *e=&s->events[j];
            fprintf(f,"%a %u %a %u\n",e->seconds,(unsigned)e->kind,(double)e->temperature_c,e->impact_count);
        }
        fprintf(f,"%u\n",h->count);
        for(unsigned j=0;j<h->count;++j) {
            const TemperatureSample *sample=&h->samples[j];
            fprintf(f,"%a %a %u\n",sample->seconds,(double)sample->temperature_c,sample->fresh);
        }
        unsigned count=r->attempt_count[i],first=count>ROUND_RETAINED_ATTEMPTS?count-ROUND_RETAINED_ATTEMPTS:0;
        for(unsigned j=first;j<count;++j) {
            const DeliveryAttempt *a=&r->attempts[i][j%ROUND_RETAINED_ATTEMPTS];
            fprintf(f,"%a %u\n",a->seconds,(unsigned)a->outcome);
        }
    }
    return !ferror(f) && fflush(f)==0;
}

bool RoundLoad(DeliveryRound *round,const char *path)
{
    if(!round || !path || !*path)return false;
    FILE *f=fopen(path,"r");if(!f)return false;
    DeliveryRound *candidate=calloc(1,sizeof(*candidate));
    bool ok=candidate && ReadRound(f,candidate);
    if(fclose(f)!=0)ok=false;
    if(ok)*round=*candidate;
    free(candidate);return ok;
}

bool RoundSave(const DeliveryRound *round,const char *path)
{
    if(!round || !path || !*path)return false;
    char temporary[4096];int n=snprintf(temporary,sizeof(temporary),"%s.tmp.XXXXXX",path);
    if(n<0 || (size_t)n>=sizeof(temporary))return false;
    int fd=mkstemp(temporary);if(fd<0)return false;
    FILE *f=fdopen(fd,"w+");
    if(!f){close(fd);unlink(temporary);return false;}
    bool ok=WriteRound(round,f);
    DeliveryRound *check=calloc(1,sizeof(*check));
    if(ok && check) {rewind(f);ok=ReadRound(f,check);}else ok=false;
    free(check);
    if(ok && fsync(fd)!=0)ok=false;
    if(fclose(f)!=0)ok=false;
    if(ok && rename(temporary,path)!=0)ok=false;
    if(!ok)unlink(temporary); /* Only our exclusively created temporary file. */
    return ok;
}

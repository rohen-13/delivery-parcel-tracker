#define _POSIX_C_SOURCE 200809L
#include "dispatch.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
const GeoPoint DISPATCH_DEPOT = {52.2595,-7.1100};
void DispatchSeed(Package p[5])
{
    const Package seed[5] = {
        {"PKG-001","People's Park",{52.2541,-7.1014},true,false},
        {"PKG-002","SETU Cork Road",{52.2458,-7.1387},true,false},
        {"PKG-003","Waterford Museum",{52.2594,-7.1060},true,false},
        {"PKG-004","Ballybricken",{52.2608,-7.1190},true,false},
        {"PKG-005","City Library",{52.2584,-7.1147},true,false}
    };
    memcpy(p,seed,sizeof(seed));
}
bool DispatchDeliver(Package p[5], int index, GeoPoint *position)
{
    if(!p || !position || index<0 || index>=5 || !p[index].included || p[index].delivered)
        return false;
    *position=p[index].point;
    p[index].delivered=true;
    p[index].included=false;
    return true;
}
static void Search(const double d[6][6], int n, int from, unsigned used,
                   int depth, double sum, int path[5], int best[5], double *minimum)
{
    if (depth == n) {
        if (sum < *minimum) { *minimum=sum; memcpy(best,path,(size_t)n*sizeof(int)); }
        return;
    }
    for (int i=0;i<n;++i) {
        double edge=d[from][i+1];
        if (!(used & (1u<<i)) && isfinite(edge) && edge>=0 && sum+edge<*minimum) {
            path[depth]=i;
            Search(d,n,i+1,used|(1u<<i),depth+1,sum+edge,path,best,minimum);
        }
    }
}
bool DispatchBestOrder(const double d[6][6], int n, int order[5], double *km)
{
    if (n<1 || n>5 || !d || !order || !km) return false;
    int path[5]; *km=INFINITY;
    Search(d,n,0,0,0,0,path,order,km);
    return isfinite(*km);
}

/* Bounded JSON tree. Each token's next index skips its complete subtree. */
typedef struct { int begin,end,next,count; char type; } Token;
typedef struct { const char *s; size_t pos; Token *t; int used; } Json;
#define TOKENS 32768
static void Space(Json *j) { while(isspace((unsigned char)j->s[j->pos])) ++j->pos; }
static int Value(Json *j,int depth)
{
    Space(j);
    if(depth>32 || j->used>=TOKENS || !j->s[j->pos]) return -1;
    int id=j->used++; Token *t=&j->t[id];
    t->begin=(int)j->pos; t->type=j->s[j->pos]; t->count=0;
    if(t->type=='{' || t->type=='[') {
        char end=t->type=='{'?'}':']'; ++j->pos; Space(j);
        if(j->s[j->pos]!=end) for(;;) {
            if(t->type=='{' && j->s[j->pos]!='"') return -1;
            if(Value(j,depth+1)<0) return -1;
            ++t->count; Space(j);
            if(t->type=='{') {
                if(j->s[j->pos++]!=':') return -1;
                if(Value(j,depth+1)<0) return -1;
                Space(j);
            }
            if(j->s[j->pos]==end) break;
            if(j->s[j->pos++]!=',') return -1;
            Space(j);
        }
        ++j->pos;
    } else if(t->type=='"') {
        ++j->pos;
        while(j->s[j->pos] && j->s[j->pos]!='"') {
            unsigned char c=(unsigned char)j->s[j->pos++];
            if(c<32) return -1;
            if(c=='\\') {
                char escape=j->s[j->pos++];
                if(!escape || !strchr("\"\\/bfnrtu",escape)) return -1;
                if(escape=='u') for(int i=0;i<4;++i)
                    if(!isxdigit((unsigned char)j->s[j->pos++])) return -1;
            }
        }
        if(j->s[j->pos++]!='"') return -1;
    } else {
        size_t begin=j->pos;
        while(j->s[j->pos] && !strchr(",]} \n\r\t",j->s[j->pos])) ++j->pos;
        if(j->pos==begin) return -1;
    }
    t->end=(int)j->pos; t->next=j->used;
    return id;
}
static bool Parse(Json *j,const char *s)
{
    if(!s || strlen(s)>1048576) return false;
    *j=(Json){.s=s,.t=calloc(TOKENS,sizeof(Token))};
    if(!j->t) return false;
    bool ok=Value(j,0)==0;
    if(ok)Space(j);
    if(!ok || j->s[j->pos]) { free(j->t); j->t=NULL; return false; }
    return true;
}
static bool Equal(Json *j,int t,const char *s)
{
    return t>=0 && j->t[t].type=='"' &&
        j->t[t].end-j->t[t].begin==(int)strlen(s)+2 &&
        strncmp(j->s+j->t[t].begin+1,s,strlen(s))==0;
}
static int Field(Json *j,int object,const char *name)
{
    if(object<0 || j->t[object].type!='{') return -1;
    for(int i=object+1;i<j->t[object].next;) {
        int value=i+1;
        if(Equal(j,i,name)) return value;
        i=j->t[value].next;
    }
    return -1;
}
static bool Number(Json *j,int t,double *number)
{
    if(t<0 || j->t[t].type=='"') return false;
    char *end; *number=strtod(j->s+j->t[t].begin,&end);
    return end==j->s+j->t[t].end && isfinite(*number) && *number>=0;
}
static bool String(Json *j,int t,char *out,size_t capacity)
{
    if(t<0 || j->t[t].type!='"') return false;
    size_t used=0;
    for(int i=j->t[t].begin+1;i<j->t[t].end-1;++i) {
        char c=j->s[i];
        if(c=='\\') {
            c=j->s[++i];
            if(c!='\\' && c!='/' && c!='"') return false;
        }
        if(used+1>=capacity) return false;
        out[used++]=c;
    }
    out[used]=0; return true;
}
bool DispatchParseMatrix(const char *s,int count,double d[6][6])
{
    if(count<1 || count>5) return false;
    Json j; if(!Parse(&j,s)) return false;
    int matrix=Field(&j,0,"sources_to_targets");
    bool ok=matrix>=0 && j.t[matrix].type=='[' && j.t[matrix].count==count+1;
    int row=ok?matrix+1:0;
    for(int i=0;ok && i<=count;++i) {
        ok=j.t[row].type=='[' && j.t[row].count==count+1;
        int cell=row+1;
        for(int k=0;ok && k<=count;++k) {
            /* A disconnected pair is not silently treated as zero distance. */
            if(!Number(&j,Field(&j,cell,"distance"),&d[i][k])) d[i][k]=INFINITY;
            cell=j.t[cell].next;
        }
        row=j.t[row].next;
    }
    free(j.t); return ok;
}
static bool Delta(const char **p,int64_t *v)
{
    uint64_t value=0; unsigned shift=0,byte;
    do {
        if(!**p || shift>30) return false;
        byte=(unsigned char)*(*p)++;
        if(byte<63 || byte>126) return false;
        byte-=63; value|=(uint64_t)(byte&31)<<shift; shift+=5;
    } while(byte&32);
    *v=(value&1)?-(int64_t)((value>>1)+1):(int64_t)(value>>1);
    return true;
}
bool DispatchParseRoute(const char *s,DeliveryRoute *r)
{
    r->ready=false; r->point_count=0;
    Json j; if(!Parse(&j,s)) return false;
    int trip=Field(&j,0,"trip"), summary=Field(&j,trip,"summary"), legs=Field(&j,trip,"legs");
    double km,seconds;
    bool ok=Number(&j,Field(&j,summary,"length"),&km) &&
        Number(&j,Field(&j,summary,"time"),&seconds) && legs>=0 &&
        j.t[legs].type=='[' && j.t[legs].count>0;
    char *shape=malloc(131072);
    if(!shape) ok=false;
    int leg=ok?legs+1:0;
    for(int i=0;ok && i<j.t[legs].count;++i) {
        ok=String(&j,Field(&j,leg,"shape"),shape,131072);
        const char *p=shape; int64_t lat=0,lon=0,a,b;
        while(ok && *p) {
            ok=Delta(&p,&a) && Delta(&p,&b);
            if(!ok || r->point_count==DISPATCH_MAX_POINTS) {ok=false;break;}
            lat+=a;lon+=b;
            GeoPoint point={lat/1e6,lon/1e6};
            if(fabs(point.lat)>85 || fabs(point.lon)>180) {ok=false;break;}
            r->points[r->point_count++]=point;
        }
        leg=j.t[leg].next;
    }
    free(shape);free(j.t);
    if(!ok || r->point_count<2) {r->point_count=0;return false;}
    r->distance_km=km;r->duration_seconds=seconds;r->ready=true;return true;
}
bool DispatchFetch(const char *url,const char *json,unsigned char *data,size_t capacity,size_t *size)
{
    if(!url || strncmp(url,"https://",8)!=0 || !data || capacity<2 || !size)return false;
    *size=0;
    int fd[2]; if(pipe(fd)!=0) return false;
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions,fd[1],STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions,fd[0]);
    posix_spawn_file_actions_addclose(&actions,fd[1]);
    char *args[22]; int n=0;
    args[n++]="/usr/bin/curl";args[n++]="--fail";args[n++]="--silent";
    args[n++]="--proto";args[n++]="=https";args[n++]="--connect-timeout";args[n++]="5";
    args[n++]="--max-time";args[n++]="20";args[n++]="--max-filesize";args[n++]="1048576";
    args[n++]="--user-agent";args[n++]="SETU-ParcelTracker-Demo/1.0 (https://github.com/rohen-13/delivery-parcel-tracker)";
    char payload[8192];
    if(json) {
        if(snprintf(payload,sizeof(payload),"json=%s",json)>=(int)sizeof(payload)) {
            posix_spawn_file_actions_destroy(&actions);close(fd[0]);close(fd[1]);return false;
        }
        args[n++]="--get";args[n++]="--data-urlencode";args[n++]=payload;
    }
    args[n++]=(char *)url;args[n]=NULL;
    pid_t pid; int error=posix_spawn(&pid,"/usr/bin/curl",&actions,NULL,args,environ);
    posix_spawn_file_actions_destroy(&actions);close(fd[1]);
    if(error) {close(fd[0]);return false;}
    unsigned char block[4096]; ssize_t got; bool overflow=false;
    while((got=read(fd[0],block,sizeof(block)))!=0) {
        if(got<0) {if(errno==EINTR)continue;overflow=true;break;}
        if(*size+(size_t)got>=capacity) overflow=true;
        if(!overflow) {memcpy(data+*size,block,(size_t)got);*size+=(size_t)got;}
    }
    close(fd[0]);int status=0;
    while(waitpid(pid,&status,0)<0) {if(errno!=EINTR)return false;}
    data[*size]=0;
    return !overflow && WIFEXITED(status) && WEXITSTATUS(status)==0 && *size>0;
}
bool DispatchPlan(const Package p[5],DeliveryRoute *r)
{
    return DispatchPlanFrom(p,DISPATCH_DEPOT,r);
}
bool DispatchPlanFrom(const Package p[5],GeoPoint position,DeliveryRoute *r)
{
    if(!r)return false;
    *r=(DeliveryRoute){0};
    snprintf(r->message,sizeof(r->message),"Invalid driver or package coordinates.");
    if(!p || !isfinite(position.lat) || !isfinite(position.lon) || fabs(position.lat)>85 || fabs(position.lon)>180)return false;
    int index[5],count=0; GeoPoint stops[6]={position};
    for(int i=0;i<5;++i) if(p[i].included && !p[i].delivered) {
        if(!isfinite(p[i].point.lat)||!isfinite(p[i].point.lon)||fabs(p[i].point.lat)>85||fabs(p[i].point.lon)>180) return false;
        index[count]=i;stops[++count]=p[i].point;
    }
    snprintf(r->message,sizeof(r->message),"Select at least one undelivered package.");
    if(!count)return false;
    char locations[2048]="[",part[96];
    for(int i=0;i<=count;++i) {
        snprintf(part,sizeof(part),"%s{\"lat\":%.6f,\"lon\":%.6f}",i?",":"",stops[i].lat,stops[i].lon);
        strcat(locations,part);
    }
    strcat(locations,"]");
    char request[6144];
    snprintf(request,sizeof(request),"{\"sources\":%s,\"targets\":%s,\"costing\":\"auto\",\"costing_options\":{\"auto\":{\"shortest\":true}},\"units\":\"kilometers\"}",locations,locations);
    unsigned char *response=malloc(1048577);size_t size;
    double d[6][6],km;int order[5];
    bool ok=response && DispatchFetch("https://valhalla1.openstreetmap.de/sources_to_targets",request,response,1048577,&size) &&
        DispatchParseMatrix((char *)response,count,d) && DispatchBestOrder((const double (*)[6])d,count,order,&km);
    if(ok) {
        /* Separate the two public-service requests by at least one second. */
        sleep(1);
        strcpy(locations,"[");
        for(int i=0;i<=count;++i) {
            GeoPoint point=i?stops[order[i-1]+1]:position;
            snprintf(part,sizeof(part),"%s{\"lat\":%.6f,\"lon\":%.6f,\"type\":\"break\"}",i?",":"",point.lat,point.lon);
            strcat(locations,part);
        }
        strcat(locations,"]");
        snprintf(request,sizeof(request),"{\"locations\":%s,\"costing\":\"auto\",\"costing_options\":{\"auto\":{\"shortest\":true}},\"units\":\"kilometers\"}",locations);
        ok=DispatchFetch("https://valhalla1.openstreetmap.de/route",request,response,1048577,&size) && DispatchParseRoute((char *)response,r);
    }
    free(response);
    if(ok) {
        r->count=count;for(int i=0;i<count;++i)r->order[i]=index[order[i]];
        snprintf(r->message,sizeof(r->message),"Distance-based driving route ready. Estimates, not live traffic.");
    } else snprintf(r->message,sizeof(r->message),"Routing unavailable or no drivable path. Check internet and retry.");
    return ok;
}

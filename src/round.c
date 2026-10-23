#include "round.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <math.h>

static double RoadMetres(GeoPoint a,GeoPoint b)
{
    const double radians=0.017453292519943295;
    double x=(b.lon-a.lon)*radians*cos((a.lat+b.lat)*radians/2);
    double y=(b.lat-a.lat)*radians;
    return hypot(x,y)*6371000;
}

static bool HasRoadLeg(const DeliveryRound *r)
{
    return r->route.ready && r->route.count>0 && r->route.leg_count==r->route.count &&
           r->route.leg_end[0]>0 && r->route.leg_end[0]<r->route.point_count;
}

bool RoundDriveSpeed(DeliveryRound *r,int multiplier)
{
    if(multiplier!=1 && multiplier!=5 && multiplier!=20)return false;
    r->run.speed=multiplier;return true;
}

bool RoundDriveStart(DeliveryRound *r)
{
    if(!HasRoadLeg(r) || r->run.state==DRIVE_ARRIVED || r->run.state==DRIVE_RUNNING)return false;
    if(r->run.state==DRIVE_READY) {
        r->run.total_m=0;r->run.distance_m=0;r->run.elapsed_seconds=0;
        for(int i=1;i<=r->route.leg_end[0];++i)
            r->run.total_m+=RoadMetres(r->route.points[i-1],r->route.points[i]);
        r->position=r->route.points[0];r->selected=r->route.order[0];
    }
    if(r->run.speed!=1 && r->run.speed!=5 && r->run.speed!=20)r->run.speed=1;
    r->run.state=DRIVE_RUNNING;return true;
}

void RoundDrivePause(DeliveryRound *r)
{
    if(r->run.state==DRIVE_RUNNING)r->run.state=DRIVE_PAUSED;
}

static void DriveUpdate(DeliveryRound *r,float seconds)
{
    if(r->run.state!=DRIVE_RUNNING || !HasRoadLeg(r) || !isfinite(seconds) || seconds<=0)return;
    double duration=r->route.leg_seconds[0];
    r->run.elapsed_seconds=fmin(duration,r->run.elapsed_seconds+seconds*r->run.speed);
    r->run.distance_m=duration>0?r->run.total_m*r->run.elapsed_seconds/duration:r->run.total_m;
    if(r->run.elapsed_seconds>=duration || r->run.total_m==0) {
        r->position=r->route.points[r->route.leg_end[0]];r->run.state=DRIVE_ARRIVED;return;
    }
    double remaining=r->run.distance_m;
    for(int i=1;i<=r->route.leg_end[0];++i) {
        GeoPoint a=r->route.points[i-1],b=r->route.points[i];
        double length=RoadMetres(a,b);
        if(length>0 && remaining<length) {
            double part=remaining/length;
            r->position=(GeoPoint){a.lat+(b.lat-a.lat)*part,a.lon+(b.lon-a.lon)*part};
            return;
        }
        remaining=fmax(0,remaining-length);
    }
    r->position=r->route.points[r->route.leg_end[0]];
    r->run.state=DRIVE_ARRIVED;
}

static bool AdvanceRoadLeg(DeliveryRound *r)
{
    if(!HasRoadLeg(r))return false;
    DeliveryRoute *route=&r->route;
    int cut=route->leg_end[0],speed=r->run.speed;
    route->distance_km=fmax(0,route->distance_km-route->leg_km[0]);
    route->duration_seconds=fmax(0,route->duration_seconds-route->leg_seconds[0]);
    --route->count;--route->leg_count;
    route->point_count-=cut;
    memmove(route->points,route->points+cut,(size_t)route->point_count*sizeof(route->points[0]));
    for(int i=0;i<route->count;++i) {
        route->order[i]=route->order[i+1];route->leg_end[i]=route->leg_end[i+1]-cut;
        route->leg_km[i]=route->leg_km[i+1];route->leg_seconds[i]=route->leg_seconds[i+1];
    }
    r->run=(DeliveryRun){.speed=speed};
    if(route->count) {r->selected=route->order[0];return true;}
    return false;
}

const char *RoundAttemptName(AttemptOutcome outcome)
{
    switch(outcome) {
    case ATTEMPT_DELIVERED:return "Delivered";
    case ATTEMPT_RECIPIENT_ABSENT:return "Recipient absent";
    case ATTEMPT_NO_ACCESS:return "No access";
    case ATTEMPT_ADDRESS_PROBLEM:return "Address problem";
    }
    return "Unknown";
}

bool RoundCanAttempt(const DeliveryRound *round)
{
    int i=round->selected;
    return round->route.ready && round->route.count>0 && round->route.order[0]==i &&
           !round->packages[i].delivered && !round->deferred[i] && round->packages[i].included &&
           round->attempt_count[i]<UINT_MAX && (round->route.point_count==0 ||
             (HasRoadLeg(round) && round->run.state==DRIVE_ARRIVED));
}

static void RecordAttempt(DeliveryRound *round,AttemptOutcome outcome)
{
    int i=round->selected;
    unsigned slot=round->attempt_count[i]++%ROUND_RETAINED_ATTEMPTS;
    round->attempts[i][slot]=(DeliveryAttempt){round->sensors[i].seconds,outcome};
}

bool RoundFailNext(DeliveryRound *round,AttemptOutcome outcome)
{
    if(!RoundCanAttempt(round) || outcome<ATTEMPT_RECIPIENT_ABSENT || outcome>ATTEMPT_ADDRESS_PROBLEM)return false;
    int i=round->selected;
    RecordAttempt(round,outcome);
    round->position=round->packages[i].point;
    round->deferred[i]=true;round->packages[i].included=false;
    if(AdvanceRoadLeg(round)) {
        snprintf(round->route.message,sizeof(round->route.message),"Attempt recorded. Start driving to the next stop.");return true;
    }
    round->route=(DeliveryRoute){0};
    round->run=(DeliveryRun){.speed=round->run.speed};
    snprintf(round->route.message,sizeof(round->route.message),"Attempt recorded: %s. Parcel deferred; replan remaining stops.",RoundAttemptName(outcome));
    return true;
}

bool RoundRetry(DeliveryRound *round,int index)
{
    if(index<0 || index>=DISPATCH_MAX_PACKAGES || !round->deferred[index] || round->packages[index].delivered)return false;
    round->deferred[index]=false;round->packages[index].included=true;
    round->route=(DeliveryRoute){0};
    round->run=(DeliveryRun){.speed=round->run.speed};
    snprintf(round->route.message,sizeof(round->route.message),"Parcel ready for retry. Plan a new route from the driver.");
    return true;
}

unsigned RoundPackageIssues(const DeliveryRound *round,int index)
{
    if(index<0 || index>=DISPATCH_MAX_PACKAGES || round->packages[index].delivered)return 0;
    const Sensors *s=&round->sensors[index];
    return (!s->connected?ROUND_ISSUE_CONNECTION:0) |
           (SensorsHeatAlert(s)?ROUND_ISSUE_HEAT:0) |
           (s->impact_count?ROUND_ISSUE_IMPACT:0) |
           (round->deferred[index]?ROUND_ISSUE_DELIVERY:0);
}

int RoundIssueCount(const DeliveryRound *round)
{
    int count=0;
    for(int i=0;i<DISPATCH_MAX_PACKAGES;++i)count+=RoundPackageIssues(round,i)!=0;
    return count;
}

bool RoundSelectNextIssue(DeliveryRound *round)
{
    for(int step=1;step<=DISPATCH_MAX_PACKAGES;++step) {
        int index=(round->selected+step)%DISPATCH_MAX_PACKAGES;
        if(RoundPackageIssues(round,index))return RoundSelect(round,index);
    }
    return false;
}

void RoundReset(DeliveryRound *round)
{
    DispatchSeed(round->packages);
    round->route=(DeliveryRoute){0};
    round->position=DISPATCH_DEPOT;
    round->selected=0;
    round->run=(DeliveryRun){.speed=1};
    for(int i=0;i<DISPATCH_MAX_PACKAGES;++i) {
        SensorsReset(&round->sensors[i]);
        TemperatureHistoryReset(&round->histories[i]);
        round->deferred[i]=false;round->attempt_count[i]=0;
        memset(round->attempts[i],0,sizeof(round->attempts[i]));
    }
}

void RoundUpdate(DeliveryRound *round,float seconds)
{
    DriveUpdate(round,seconds);
    for(int i=0;i<DISPATCH_MAX_PACKAGES;++i) {
        if(round->packages[i].delivered)continue;
        SensorsUpdate(&round->sensors[i],seconds);
        Sensors *s=&round->sensors[i];
        TemperatureHistoryObserve(&round->histories[i],s->seconds,s->temperature_c,s->connected);
    }
}

bool RoundSelect(DeliveryRound *round,int index)
{
    if(index<0 || index>=DISPATCH_MAX_PACKAGES)return false;
    round->selected=index;
    return true;
}

bool RoundSetIncluded(DeliveryRound *round,int index,bool included)
{
    if(index<0 || index>=DISPATCH_MAX_PACKAGES || round->packages[index].delivered || round->deferred[index])return false;
    if(round->packages[index].included==included)return true;
    round->packages[index].included=included;
    round->route=(DeliveryRoute){0};
    round->run=(DeliveryRun){.speed=round->run.speed};
    snprintf(round->route.message,sizeof(round->route.message),"Package selection changed. Plan the updated route.");
    return true;
}

bool RoundDeliverNext(DeliveryRound *round)
{
    if(!RoundCanAttempt(round))return false;
    if(!DispatchDeliver(round->packages,round->selected,&round->position))return false;
    RecordAttempt(round,ATTEMPT_DELIVERED);
    if(AdvanceRoadLeg(round)) {
        snprintf(round->route.message,sizeof(round->route.message),"Delivery recorded. Start driving to the next stop.");return true;
    }
    round->route=(DeliveryRoute){0};
    round->run=(DeliveryRun){.speed=round->run.speed};
    int remaining=0;
    for(int i=0;i<5;++i)remaining+=!round->packages[i].delivered;
    snprintf(round->route.message,sizeof(round->route.message),remaining?
             "Delivery recorded. Plan the remaining stops from the driver.":
             "All packages delivered. Condition histories are available for export.");
    return true;
}

bool RoundWriteCSV(const DeliveryRound *round,FILE *output)
{
    if(!output || fprintf(output,"package_id,elapsed_seconds,event,temperature_c,impact_count\n")<0)return false;
    const Sensors *s=&round->sensors[round->selected];
    /* Package IDs are the fixed PKG-001 through PKG-005 identifiers. */
    for(unsigned i=0;i<s->event_count;++i) {
        const SensorEvent *e=&s->events[i];
        if(fprintf(output,"\"%s\",%.3f,\"%s\",%.1f,%u\n",round->packages[round->selected].id,
                   e->seconds,SensorsEventName(e->kind),e->temperature_c,e->impact_count)<0)return false;
    }
    return fflush(output)==0 && !ferror(output);
}

static bool CSVText(FILE *output,const char *text)
{
    if(fputc('"',output)==EOF)return false;
    for(;*text;++text) {
        if(*text=='"' && fputc('"',output)==EOF)return false;
        if(fputc(*text,output)==EOF)return false;
    }
    return fputc('"',output)!=EOF;
}

bool RoundWriteAttemptsCSV(const DeliveryRound *round,FILE *output)
{
    if(!output || fprintf(output,"package_id,attempt_number,elapsed_seconds,outcome\n")<0)return false;
    int i=round->selected;
    unsigned count=round->attempt_count[i];
    unsigned first=count>ROUND_RETAINED_ATTEMPTS?count-ROUND_RETAINED_ATTEMPTS:0;
    for(unsigned n=first;n<count;++n) {
        const DeliveryAttempt *a=&round->attempts[i][n%ROUND_RETAINED_ATTEMPTS];
        if(fprintf(output,"\"%s\",%u,%.3f,\"%s\"\n",round->packages[i].id,n+1,a->seconds,RoundAttemptName(a->outcome))<0)return false;
    }
    return fflush(output)==0 && !ferror(output);
}

bool RoundWriteSummaryCSV(const DeliveryRound *round,FILE *output)
{
    if(!output || fprintf(output,"package_id,destination,delivery_status,planned_stop,sensor_status,temperature_c,peak_temperature_c,impact_count,heat_exposure_seconds,unobserved_seconds,active_flags,attempt_count,last_outcome\n")<0)return false;
    for(int i=0;i<DISPATCH_MAX_PACKAGES;++i) {
        const Package *p=&round->packages[i];
        const Sensors *s=&round->sensors[i];
        int stop=0;
        if(!p->delivered && round->route.ready)
            for(int j=0;j<round->route.count;++j)if(round->route.order[j]==i)stop=j+1;
        if(!CSVText(output,p->id) || fputc(',',output)==EOF || !CSVText(output,p->destination))return false;
        if(fprintf(output,",%s,",p->delivered?"delivered":round->deferred[i]?"deferred":p->included?"pending":"excluded")<0)return false;
        if(stop && fprintf(output,"%d",stop)<0)return false;
        unsigned flags=RoundPackageIssues(round,i);
        unsigned attempts=round->attempt_count[i];
        const char *last=attempts?RoundAttemptName(round->attempts[i][(attempts-1)%ROUND_RETAINED_ATTEMPTS].outcome):"";
        if(fprintf(output,",%s,%.1f,%.1f,%u,%.1f,%.1f,\"%s%s%s%s\",%u,\"%s\"\n",
                   p->delivered?"archived":s->connected?"connected":"disconnected",
                   s->temperature_c,s->peak_temperature_c,s->impact_count,s->heat_seconds,s->unobserved_seconds,
                   flags&ROUND_ISSUE_CONNECTION?"offline;":"",flags&ROUND_ISSUE_HEAT?"heat;":"",
                   flags&ROUND_ISSUE_IMPACT?"impact history;":"",flags&ROUND_ISSUE_DELIVERY?"delivery deferred;":"",attempts,last)<0)return false;
    }
    return fflush(output)==0 && !ferror(output);
}

static bool ExportCSV(const DeliveryRound *round,const char *directory,char *name,size_t capacity,int kind)
{
    if(!name || !capacity)return false;
    name[0]=0;
    if(!directory || !directory[0])return false;
    for(unsigned n=1;n<=999;++n) {
        char candidate[64],path[4096];
        int length=kind==1?snprintf(candidate,sizeof(candidate),"round-summary-%03u.csv",n):
            snprintf(candidate,sizeof(candidate),"%s-%s-%03u.csv",round->packages[round->selected].id,kind==2?"attempts":"events",n);
        if(length<0 || (size_t)length>=capacity)return false;
        length=snprintf(path,sizeof(path),"%s%s%s",directory,directory[strlen(directory)-1]=='/'?"":"/",candidate);
        if(length<0 || (size_t)length>=sizeof(path))return false;
        FILE *output=fopen(path,"wx");
        if(!output){if(errno==EEXIST)continue;return false;}
        bool written=kind==1?RoundWriteSummaryCSV(round,output):kind==2?RoundWriteAttemptsCSV(round,output):RoundWriteCSV(round,output);
        int closed=fclose(output);
        if(!written || closed!=0)return false;
        strcpy(name,candidate);return true;
    }
    return false;
}

bool RoundExportCSV(const DeliveryRound *round,const char *directory,char *name,size_t capacity)
{
    return ExportCSV(round,directory,name,capacity,0);
}

bool RoundExportSummaryCSV(const DeliveryRound *round,const char *directory,char *name,size_t capacity)
{
    return ExportCSV(round,directory,name,capacity,1);
}

bool RoundExportAttemptsCSV(const DeliveryRound *round,const char *directory,char *name,size_t capacity)
{
    return ExportCSV(round,directory,name,capacity,2);
}

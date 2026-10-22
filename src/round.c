#include "round.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

unsigned RoundPackageIssues(const DeliveryRound *round,int index)
{
    if(index<0 || index>=DISPATCH_MAX_PACKAGES || round->packages[index].delivered)return 0;
    const Sensors *s=&round->sensors[index];
    return (!s->connected?ROUND_ISSUE_CONNECTION:0) |
           (SensorsHeatAlert(s)?ROUND_ISSUE_HEAT:0) |
           (s->impact_count?ROUND_ISSUE_IMPACT:0);
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
    for(int i=0;i<DISPATCH_MAX_PACKAGES;++i) {
        SensorsReset(&round->sensors[i]);
        TemperatureHistoryReset(&round->histories[i]);
    }
}

void RoundUpdate(DeliveryRound *round,float seconds)
{
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
    if(index<0 || index>=DISPATCH_MAX_PACKAGES || round->packages[index].delivered)return false;
    if(round->packages[index].included==included)return true;
    round->packages[index].included=included;
    round->route=(DeliveryRoute){0};
    snprintf(round->route.message,sizeof(round->route.message),"Package selection changed. Plan the updated route.");
    return true;
}

bool RoundDeliverNext(DeliveryRound *round)
{
    if(!round->route.ready || round->route.count<1 || round->selected!=round->route.order[0])return false;
    if(!DispatchDeliver(round->packages,round->selected,&round->position))return false;
    round->route=(DeliveryRoute){0};
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

bool RoundWriteSummaryCSV(const DeliveryRound *round,FILE *output)
{
    if(!output || fprintf(output,"package_id,destination,delivery_status,planned_stop,sensor_status,temperature_c,peak_temperature_c,impact_count,heat_exposure_seconds,unobserved_seconds,active_flags\n")<0)return false;
    for(int i=0;i<DISPATCH_MAX_PACKAGES;++i) {
        const Package *p=&round->packages[i];
        const Sensors *s=&round->sensors[i];
        int stop=0;
        if(!p->delivered && round->route.ready)
            for(int j=0;j<round->route.count;++j)if(round->route.order[j]==i)stop=j+1;
        if(!CSVText(output,p->id) || fputc(',',output)==EOF || !CSVText(output,p->destination))return false;
        if(fprintf(output,",%s,",p->delivered?"delivered":p->included?"pending":"excluded")<0)return false;
        if(stop && fprintf(output,"%d",stop)<0)return false;
        unsigned flags=RoundPackageIssues(round,i);
        if(fprintf(output,",%s,%.1f,%.1f,%u,%.1f,%.1f,\"%s%s%s\"\n",
                   p->delivered?"archived":s->connected?"connected":"disconnected",
                   s->temperature_c,s->peak_temperature_c,s->impact_count,s->heat_seconds,s->unobserved_seconds,
                   flags&ROUND_ISSUE_CONNECTION?"offline;":"",flags&ROUND_ISSUE_HEAT?"heat;":"",
                   flags&ROUND_ISSUE_IMPACT?"impact history;":"")<0)return false;
    }
    return fflush(output)==0 && !ferror(output);
}

static bool ExportCSV(const DeliveryRound *round,const char *directory,char *name,size_t capacity,bool summary)
{
    if(!name || !capacity)return false;
    name[0]=0;
    if(!directory || !directory[0])return false;
    for(unsigned n=1;n<=999;++n) {
        char candidate[64],path[4096];
        int length=summary?snprintf(candidate,sizeof(candidate),"round-summary-%03u.csv",n):
            snprintf(candidate,sizeof(candidate),"%s-events-%03u.csv",round->packages[round->selected].id,n);
        if(length<0 || (size_t)length>=capacity)return false;
        length=snprintf(path,sizeof(path),"%s%s%s",directory,directory[strlen(directory)-1]=='/'?"":"/",candidate);
        if(length<0 || (size_t)length>=sizeof(path))return false;
        FILE *output=fopen(path,"wx");
        if(!output){if(errno==EEXIST)continue;return false;}
        bool written=summary?RoundWriteSummaryCSV(round,output):RoundWriteCSV(round,output);
        int closed=fclose(output);
        if(!written || closed!=0)return false;
        strcpy(name,candidate);return true;
    }
    return false;
}

bool RoundExportCSV(const DeliveryRound *round,const char *directory,char *name,size_t capacity)
{
    return ExportCSV(round,directory,name,capacity,false);
}

bool RoundExportSummaryCSV(const DeliveryRound *round,const char *directory,char *name,size_t capacity)
{
    return ExportCSV(round,directory,name,capacity,true);
}

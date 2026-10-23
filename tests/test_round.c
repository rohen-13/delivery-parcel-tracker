#include "../src/round.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static DeliveryRound round;
int main(int argc,char **argv)
{
    RoundReset(&round);
    assert(!RoundFailNext(&round,ATTEMPT_NO_ACCESS));
    round.route.ready=true;round.route.count=2;round.route.order[0]=0;round.route.order[1]=1;
    assert(!RoundFailNext(&round,ATTEMPT_DELIVERED));
    RoundSelect(&round,1);assert(!RoundFailNext(&round,ATTEMPT_NO_ACCESS));
    RoundSelect(&round,0);RoundUpdate(&round,2);
    assert(RoundFailNext(&round,ATTEMPT_RECIPIENT_ABSENT));
    assert(round.deferred[0] && !round.packages[0].delivered && !round.packages[0].included);
    assert(!round.route.ready && round.position.lat==round.packages[0].point.lat);
    assert(round.attempt_count[0]==1 && round.attempts[0][0].seconds==2);
    assert(RoundPackageIssues(&round,0)&ROUND_ISSUE_DELIVERY);
    assert(!RoundSetIncluded(&round,0,true));
    RoundUpdate(&round,2);assert(round.sensors[0].seconds==4);
    assert(RoundRetry(&round,0) && round.packages[0].included && !round.deferred[0]);
    assert(!RoundRetry(&round,0) && !RoundRetry(&round,5));
    round.route.ready=true;round.route.count=1;round.route.order[0]=0;
    assert(RoundDeliverNext(&round) && round.attempt_count[0]==2);
    assert(round.attempts[0][1].outcome==ATTEMPT_DELIVERED);
    assert(!RoundRetry(&round,0) && RoundPackageIssues(&round,0)==0);
    FILE *attempt_csv=tmpfile();assert(attempt_csv && RoundWriteAttemptsCSV(&round,attempt_csv));
    rewind(attempt_csv);char attempt_text[2048]={0};
    assert(fread(attempt_text,1,sizeof(attempt_text)-1,attempt_csv)>0);fclose(attempt_csv);
    assert(strstr(attempt_text,"Recipient absent") && strstr(attempt_text,"Delivered"));
    RoundReset(&round);
    for(int i=0;i<10;++i) {
        round.route.ready=true;round.route.count=1;round.route.order[0]=0;
        assert(RoundFailNext(&round,ATTEMPT_ADDRESS_PROBLEM));
        assert(RoundRetry(&round,0));
    }
    assert(round.attempt_count[0]==10);
    attempt_csv=tmpfile();assert(attempt_csv && RoundWriteAttemptsCSV(&round,attempt_csv));
    rewind(attempt_csv);memset(attempt_text,0,sizeof(attempt_text));
    assert(fread(attempt_text,1,sizeof(attempt_text)-1,attempt_csv)>0);fclose(attempt_csv);
    assert(strstr(attempt_text,"\"PKG-001\",3,") && strstr(attempt_text,"\"PKG-001\",10,"));
    assert(!strstr(attempt_text,"\"PKG-001\",1,"));
    round.route.ready=true;round.route.count=1;round.route.order[0]=0;
    assert(!RoundFailNext(&round,(AttemptOutcome)99));
    assert(round.attempt_count[0]==10 && round.route.ready);
    assert(RoundFailNext(&round,ATTEMPT_NO_ACCESS));
    attempt_csv=tmpfile();assert(attempt_csv && RoundWriteSummaryCSV(&round,attempt_csv));
    rewind(attempt_csv);memset(attempt_text,0,sizeof(attempt_text));
    assert(fread(attempt_text,1,sizeof(attempt_text)-1,attempt_csv)>0);fclose(attempt_csv);
    assert(strstr(attempt_text,",deferred,,connected,") && strstr(attempt_text,",11,\"No access\""));
    RoundReset(&round);assert(round.attempt_count[0]==0 && !round.deferred[0]);
    assert(RoundIssueCount(&round)==0 && !RoundSelectNextIssue(&round));
    assert(round.selected==0 && RoundPackageIssues(&round,-1)==0);
    SensorsTriggerImpact(&round.sensors[2]);
    SensorsSetConnected(&round.sensors[4],false);
    assert(RoundIssueCount(&round)==2);
    assert(RoundSelectNextIssue(&round) && round.selected==2);
    assert(RoundSelectNextIssue(&round) && round.selected==4);
    assert(RoundSelectNextIssue(&round) && round.selected==2);
    round.packages[2].delivered=true;
    assert(RoundIssueCount(&round)==1 && RoundPackageIssues(&round,2)==0);
    SensorsSetConnected(&round.sensors[4],true);
    assert(!RoundSelectNextIssue(&round) && round.selected==2);
    SensorsTriggerHeat(&round.sensors[0]);RoundUpdate(&round,6);
    assert(RoundPackageIssues(&round,0)==ROUND_ISSUE_HEAT);
    SensorsTriggerImpact(&round.sensors[0]);
    assert(RoundPackageIssues(&round,0)==(ROUND_ISSUE_HEAT|ROUND_ISSUE_IMPACT));
    assert(RoundIssueCount(&round)==1);
    RoundReset(&round);
    assert(round.selected==0 && round.position.lat==DISPATCH_DEPOT.lat);
    assert(!RoundSelect(&round,-1) && !RoundSelect(&round,5));
    SensorsTriggerHeat(&round.sensors[0]);
    SensorsTriggerImpact(&round.sensors[0]);
    RoundUpdate(&round,6);
    assert(round.sensors[0].temperature_c==32 && round.sensors[0].impact_count==1);
    assert(round.sensors[1].temperature_c==20 && round.sensors[1].impact_count==0);
    assert(RoundSelect(&round,1));
    SensorsSetConnected(&round.sensors[1],false);
    RoundUpdate(&round,1);
    assert(round.histories[0].count && round.histories[1].count);
    assert(round.sensors[1].stale_seconds==1 && round.sensors[0].connected);
    FILE *csv=tmpfile();assert(csv && RoundWriteCSV(&round,csv));
    rewind(csv);char report[2048]={0};assert(fread(report,1,sizeof(report)-1,csv)>0);
    assert(strstr(report,"package_id,elapsed_seconds,event,temperature_c,impact_count"));
    assert(strstr(report,"PKG-002") && !strstr(report,"PKG-001"));fclose(csv);
    assert(!RoundWriteCSV(&round,NULL));
    assert(!RoundDeliverNext(&round));
    round.route.ready=true;round.route.count=2;round.route.order[0]=0;round.route.order[1]=1;
    assert(!RoundDeliverNext(&round)); /* A different package cannot be completed. */
    assert(RoundSelect(&round,0) && RoundDeliverNext(&round));
    assert(round.packages[0].delivered && !round.route.ready);
    assert(round.position.lat==round.packages[0].point.lat);
    double frozen=round.sensors[0].seconds;
    RoundUpdate(&round,2);
    assert(round.sensors[0].seconds==frozen); /* Delivery archives its condition data. */
    assert(!RoundSetIncluded(&round,0,true));
    round.route.ready=true;round.route.count=1;round.route.order[0]=1;
    csv=tmpfile();assert(csv && RoundWriteSummaryCSV(&round,csv));
    rewind(csv);memset(report,0,sizeof(report));
    assert(fread(report,1,sizeof(report)-1,csv)>0);fclose(csv);
    assert(strstr(report,"package_id,destination,delivery_status,planned_stop,sensor_status"));
    assert(strstr(report,"\"PKG-001\",\"People's Park\",delivered,,archived"));
    assert(strstr(report,"\"PKG-002\",\"SETU Cork Road\",pending,1,disconnected"));
    assert(strstr(report,"PKG-005") && !RoundWriteSummaryCSV(&round,NULL));
    round.route.ready=true;
    assert(RoundSetIncluded(&round,1,false) && !round.route.ready);
    RoundReset(&round);
    for(int i=0;i<5;++i) {
        assert(!round.packages[i].delivered && round.packages[i].included);
        assert(round.sensors[i].connected && round.sensors[i].impact_count==0);
        assert(round.histories[i].count==0);
    }
    char name[64]="old";
    assert(!RoundExportCSV(&round,"/definitely-missing-round-export",name,sizeof(name)) && !name[0]);
    if(argc==2) {
        char first[64],second[64],path[4096];
        assert(RoundExportAttemptsCSV(&round,argv[1],first,sizeof(first)));
        assert(RoundExportAttemptsCSV(&round,argv[1],second,sizeof(second)));
        assert(strcmp(first,second)!=0 && strstr(first,"PKG-001-attempts-"));
        assert(!RoundExportAttemptsCSV(&round,"/definitely-missing-round-export",name,sizeof(name)));
        assert(RoundExportSummaryCSV(&round,argv[1],first,sizeof(first)));
        assert(RoundExportSummaryCSV(&round,argv[1],second,sizeof(second)));
        assert(strcmp(first,second)!=0 && strstr(first,"round-summary-"));
        assert(!RoundExportSummaryCSV(&round,argv[1],name,2) && !name[0]);
        assert(!RoundExportSummaryCSV(&round,"/definitely-missing-round-export",name,sizeof(name)) && !name[0]);
        RoundSelect(&round,1);SensorsTriggerImpact(&round.sensors[1]);
        assert(RoundExportCSV(&round,argv[1],first,sizeof(first)));
        assert(RoundExportCSV(&round,argv[1],second,sizeof(second)));
        assert(strcmp(first,second)!=0 && strstr(first,"PKG-002-events-"));
        snprintf(path,sizeof(path),"%s/%s",argv[1],first);
        FILE *saved=fopen(path,"r");assert(saved);
        char contents[2048]={0};assert(fread(contents,1,sizeof(contents)-1,saved)>0);fclose(saved);
        assert(strstr(contents,"PKG-002") && strstr(contents,"Impact recorded"));
        RoundReset(&round);
        assert(RoundExportCSV(&round,argv[1],name,sizeof(name)) && strstr(name,"PKG-001-events-"));
        saved=fopen(path,"r");assert(saved);char preserved[2048]={0};
        assert(fread(preserved,1,sizeof(preserved)-1,saved)>0);fclose(saved);
        assert(strcmp(contents,preserved)==0);
    }
    puts("Round checks passed: package isolation, selection, route invalidation, delivery archiving and reset.");
}

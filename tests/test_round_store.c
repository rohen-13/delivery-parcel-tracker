#include "../src/round_store.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static DeliveryRound original, restored, before;
int main(int argc,char **argv)
{
    assert(argc==2);
    char path[4096];snprintf(path,sizeof(path),"%s/round.save",argv[1]);
    RoundReset(&original);
    SensorsTriggerHeat(&original.sensors[2]);RoundUpdate(&original,6);
    SensorsTriggerImpact(&original.sensors[2]);SensorsSetConnected(&original.sensors[3],false);
    RoundUpdate(&original,2);
    original.route.ready=true;original.route.count=1;original.route.order[0]=0;
    assert(RoundFailNext(&original,ATTEMPT_NO_ACCESS));
    RoundSelect(&original,1);original.route.ready=true;original.route.count=1;original.route.order[0]=1;
    assert(RoundDeliverNext(&original));
    RoundSelect(&original,3);
    assert(RoundSave(&original,path));
    RoundReset(&restored);assert(RoundLoad(&restored,path));
    assert(restored.selected==3 && restored.position.lat==original.position.lat);
    assert(restored.deferred[0] && !restored.packages[0].included);
    assert(restored.packages[1].delivered && restored.attempt_count[1]==1);
    assert(restored.attempts[0][0].outcome==ATTEMPT_NO_ACCESS);
    assert(restored.sensors[2].temperature_c==original.sensors[2].temperature_c);
    assert(restored.sensors[2].impact_count==1 && restored.sensors[2].heating);
    assert(!restored.sensors[3].connected && restored.sensors[3].stale_seconds==2);
    assert(restored.histories[2].count==original.histories[2].count);
    assert(restored.sensors[2].event_count==original.sensors[2].event_count);
    assert(!restored.route.ready && strstr(restored.route.message,"Plan"));
    assert(restored.sensors[2].seconds==8); /* Loading adds no wall-clock elapsed time. */
    before=restored;
    original.sensors[2].temperature_c=NAN;
    assert(!RoundSave(&original,path));
    assert(RoundLoad(&restored,path)); /* Invalid state cannot replace a valid checkpoint. */
    original.sensors[2].temperature_c=restored.sensors[2].temperature_c;
    original.sensors[2].event_count=SENSOR_EVENT_CAPACITY+1;
    assert(!RoundSave(&original,path));
    original.sensors[2].event_count=restored.sensors[2].event_count;
    assert(RoundLoad(&restored,path));
    assert(!RoundLoad(&restored,"/not-a-real-round-file"));
    assert(memcmp(&restored,&before,sizeof(before))==0);
    FILE *file=fopen(path,"w");assert(file);fputs("PARCEL_ROUND 1\n99999999999999999999999999999 0 0\n",file);fclose(file);
    assert(!RoundLoad(&restored,path));
    assert(memcmp(&restored,&before,sizeof(before))==0);
    assert(RoundSave(&original,path));
    assert(!RoundSave(&original,"/missing-round-directory/file"));
    assert(RoundLoad(&restored,path)); /* A failed save does not damage the earlier snapshot. */
    file=fopen(path,"w");assert(file);fputs("PARCEL_ROUND 99\n",file);fclose(file);
    before=restored;assert(!RoundLoad(&restored,path));
    assert(memcmp(&restored,&before,sizeof(before))==0);
    file=fopen(path,"w");assert(file);fputs("PARCEL_ROUND 1\n3 nan 0\n",file);fclose(file);
    assert(!RoundLoad(&restored,path));
    assert(memcmp(&restored,&before,sizeof(before))==0);
    assert(RoundSave(&original,path));
    file=fopen(path,"a");assert(file);fputs("TRAILING_JUNK",file);fclose(file);
    assert(!RoundLoad(&restored,path));
    assert(memcmp(&restored,&before,sizeof(before))==0);
    puts("Save/resume checks passed: state, histories, attempts, paused time, invalid files and transactional load.");
}

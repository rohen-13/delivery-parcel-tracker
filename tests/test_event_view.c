#include "../src/event_view.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>

int main(void)
{
    Sensors s;
    SensorsReset(&s);
    EventView v=EventViewBuild(&s,EVENT_FILTER_ALL,0);
    assert(v.total==1 && v.count==1 && v.indices[0]==0);
    v=EventViewBuild(&s,EVENT_FILTER_HEAT,UINT_MAX);
    assert(v.total==0 && v.count==0 && v.page==0 && v.pages==0);
    SensorsTriggerHeat(&s);
    SensorsUpdate(&s,6);
    SensorsStopHeat(&s);
    SensorsUpdate(&s,6);
    SensorsSetConnected(&s,false);
    SensorsSetConnected(&s,true);
    for(int i=0;i<12;++i)SensorsTriggerImpact(&s);
    v=EventViewBuild(&s,EVENT_FILTER_HEAT,0);
    assert(v.total==4 && v.count==4 && v.pages==1);
    assert(s.events[v.indices[0]].kind==SENSOR_HEAT_CLEARED);
    v=EventViewBuild(&s,EVENT_FILTER_CONNECTION,0);
    assert(v.total==2 && s.events[v.indices[0]].kind==SENSOR_RECONNECTED);
    v=EventViewBuild(&s,EVENT_FILTER_IMPACT,0);
    assert(v.total==12 && v.pages==3 && v.count==5);
    unsigned newest=v.indices[0], last=v.indices[4];
    v=EventViewBuild(&s,EVENT_FILTER_IMPACT,1);
    assert(v.count==5 && v.indices[0]<last);
    v=EventViewBuild(&s,EVENT_FILTER_IMPACT,UINT_MAX);
    assert(v.page==2 && v.count==2 && v.indices[0]<newest);
    unsigned before=s.event_count;
    for(int i=0;i<50;++i)SensorsTriggerImpact(&s);
    v=EventViewBuild(&s,EVENT_FILTER_ALL,0);
    assert(v.total==32 && v.indices[0]==31 && before==19);
    SensorsReset(&s);
    v=EventViewBuild(&s,EVENT_FILTER_IMPACT,2);
    assert(v.total==0 && v.page==0);
    puts("Event view checks passed: categories, ordering, pagination, eviction and reset.");
    return 0;
}

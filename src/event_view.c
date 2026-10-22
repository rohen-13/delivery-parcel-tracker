#include "event_view.h"

static bool Matches(SensorEventKind kind, EventFilter filter)
{
    switch (filter) {
        case EVENT_FILTER_ALL: return true;
        case EVENT_FILTER_HEAT:
            return kind == SENSOR_HEAT_STARTED || kind == SENSOR_HEAT_STOPPED ||
                   kind == SENSOR_HEAT_RAISED || kind == SENSOR_HEAT_CLEARED;
        case EVENT_FILTER_IMPACT: return kind == SENSOR_IMPACT;
        case EVENT_FILTER_CONNECTION:
            return kind == SENSOR_DISCONNECTED || kind == SENSOR_RECONNECTED;
    }
    return false;
}

EventView EventViewBuild(const Sensors *sensors, EventFilter filter, unsigned page)
{
    EventView view = {0};
    unsigned matches[SENSOR_EVENT_CAPACITY];
    for (unsigned i=sensors->event_count; i>0; --i)
        if (Matches(sensors->events[i-1].kind,filter)) matches[view.total++]=i-1;
    view.pages=(view.total+EVENT_VIEW_PAGE_SIZE-1)/EVENT_VIEW_PAGE_SIZE;
    view.page=view.pages && page>=view.pages ? view.pages-1 : view.pages ? page : 0;
    unsigned start=view.page*EVENT_VIEW_PAGE_SIZE;
    for (unsigned i=start; i<view.total && view.count<EVENT_VIEW_PAGE_SIZE; ++i)
        view.indices[view.count++]=matches[i];
    return view;
}

#ifndef EVENT_VIEW_H
#define EVENT_VIEW_H
#include "sensors.h"

typedef enum { EVENT_FILTER_ALL, EVENT_FILTER_HEAT, EVENT_FILTER_IMPACT,
               EVENT_FILTER_CONNECTION } EventFilter;
#define EVENT_VIEW_PAGE_SIZE 5

typedef struct {
    unsigned indices[EVENT_VIEW_PAGE_SIZE];
    unsigned count, total, page, pages;
} EventView;
/* Newest events first; pages are clamped to the currently retained history. */
EventView EventViewBuild(const Sensors *sensors, EventFilter filter, unsigned page);
#endif

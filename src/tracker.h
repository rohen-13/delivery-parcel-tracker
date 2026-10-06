#ifndef TRACKER_H
#define TRACKER_H

#include <stdbool.h>

/* Shared interface: agree changes together before editing this file. */
typedef struct {
    float elapsed_seconds;
    bool running;
} Tracker;

void TrackerReset(Tracker *tracker);
void TrackerStart(Tracker *tracker);
void TrackerPause(Tracker *tracker);
void TrackerUpdate(Tracker *tracker, float delta_seconds);
float TrackerProgress(const Tracker *tracker);
const char *TrackerStatus(const Tracker *tracker);

#endif

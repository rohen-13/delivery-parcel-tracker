#include "tracker.h"
#include <math.h>

static const float JOURNEY_SECONDS = 20.0f;

void TrackerReset(Tracker *tracker)
{
    tracker->elapsed_seconds = 0.0f;
    tracker->running = false;
}

void TrackerStart(Tracker *tracker)
{
    if (tracker->elapsed_seconds < JOURNEY_SECONDS) tracker->running = true;
}

void TrackerPause(Tracker *tracker)
{
    tracker->running = false;
}

void TrackerUpdate(Tracker *tracker, float delta_seconds)
{
    if (!tracker->running || !isfinite(delta_seconds) || delta_seconds <= 0.0f) return;
    tracker->elapsed_seconds += delta_seconds;
    if (tracker->elapsed_seconds >= JOURNEY_SECONDS) {
        tracker->elapsed_seconds = JOURNEY_SECONDS;
        tracker->running = false;
    }
}

float TrackerProgress(const Tracker *tracker)
{
    return tracker->elapsed_seconds / JOURNEY_SECONDS;
}

const char *TrackerStatus(const Tracker *tracker)
{
    if (TrackerProgress(tracker) >= 1.0f) return "Delivered";
    if (tracker->running) return "In transit";
    if (tracker->elapsed_seconds > 0.0f) return "Paused";
    return "At depot";
}

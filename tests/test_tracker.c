#include "../src/tracker.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    Tracker t;
    TrackerReset(&t);
    assert(TrackerProgress(&t) == 0.0f && !t.running);
    assert(strcmp(TrackerStatus(&t), "At depot") == 0);
    TrackerUpdate(&t, 10.0f);
    assert(TrackerProgress(&t) == 0.0f);
    TrackerStart(&t);
    TrackerUpdate(&t, 5.0f);
    assert(TrackerProgress(&t) == 0.25f);
    assert(strcmp(TrackerStatus(&t), "In transit") == 0);
    TrackerPause(&t);
    TrackerUpdate(&t, 10.0f);
    assert(TrackerProgress(&t) == 0.25f);
    assert(strcmp(TrackerStatus(&t), "Paused") == 0);
    TrackerStart(&t);
    TrackerUpdate(&t, -1.0f);
    assert(TrackerProgress(&t) == 0.25f);
    TrackerUpdate(&t, 30.0f);
    assert(TrackerProgress(&t) == 1.0f && !t.running);
    assert(strcmp(TrackerStatus(&t), "Delivered") == 0);
    TrackerStart(&t);
    assert(!t.running);
    TrackerReset(&t);
    assert(TrackerProgress(&t) == 0.0f && !t.running);
    TrackerStart(&t);
    TrackerUpdate(&t, 2.0f);
    TrackerReset(&t);
    assert(TrackerProgress(&t) == 0.0f && !t.running);
    puts("Tracking checks passed: start, pause, resume, arrival, reset and time bounds.");
    return 0;
}

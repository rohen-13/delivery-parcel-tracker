#include "../src/tracker.h"
#include "../src/sensors.h"
#include <assert.h>
#include <stdio.h>

static void Update(Tracker *tracker, Sensors *sensors, float seconds)
{
    TrackerUpdate(tracker, seconds);
    SensorsUpdate(sensors, seconds);
}

int main(void)
{
    Tracker tracker;
    Sensors sensors;
    TrackerReset(&tracker);
    SensorsReset(&sensors);
    TrackerStart(&tracker);
    SensorsTriggerHeat(&sensors);
    Update(&tracker, &sensors, 2);
    assert(tracker.elapsed_seconds == 2 && sensors.temperature_c == 24);
    TrackerPause(&tracker);
    Update(&tracker, &sensors, 3);
    assert(tracker.elapsed_seconds == 2 && SensorsHeatAlert(&sensors));
    assert(sensors.peak_temperature_c == 30 && sensors.heat_seconds == 0);
    SensorsSetConnected(&sensors, false);
    TrackerStart(&tracker);
    Update(&tracker, &sensors, 18);
    assert(TrackerProgress(&tracker) == 1 && !tracker.running);
    assert(sensors.temperature_c == 30 && sensors.stale_seconds == 18);
    assert(sensors.unobserved_seconds == 18 && sensors.heat_seconds == 0);
    SensorsTriggerImpact(&sensors);
    assert(sensors.impact_count == 0);
    SensorsSetConnected(&sensors, true);
    Update(&tracker, &sensors, 5);
    assert(sensors.temperature_c == 40 && sensors.stale_seconds == 0);
    assert(TrackerProgress(&tracker) == 1);
    assert(sensors.peak_temperature_c == 40 && sensors.heat_seconds == 5);
    SensorsTriggerImpact(&sensors);
    SensorsStopHeat(&sensors);
    Update(&tracker, &sensors, 6);
    assert(!SensorsHeatAlert(&sensors) && sensors.impact_count == 1);
    assert(sensors.heat_seconds == 10 && sensors.unobserved_seconds == 18);
    TrackerReset(&tracker);
    SensorsReset(&sensors);
    assert(TrackerProgress(&tracker) == 0 && !tracker.running);
    assert(sensors.connected && sensors.temperature_c == 20);
    assert(sensors.impact_count == 0 && sensors.event_count == 1);
    assert(sensors.peak_temperature_c == 20 && sensors.heat_seconds == 0);
    assert(sensors.unobserved_seconds == 0);
    puts("Journey checks passed: pause, independent monitoring, arrival, reconnect and reset.");
}

#include "sensors.h"
#include <limits.h>
#include <math.h>
#include <string.h>

static const float BASELINE_C = 20.0f;
static const float HEAT_TARGET_C = 40.0f;
static const float ALERT_C = 30.0f;
static const float CHANGE_C_PER_SECOND = 2.0f;

static void Record(Sensors *sensors, SensorEventKind kind, double seconds)
{
    if (sensors->event_count == SENSOR_EVENT_CAPACITY) {
        memmove(sensors->events, sensors->events + 1,
                (SENSOR_EVENT_CAPACITY - 1) * sizeof(sensors->events[0]));
        --sensors->event_count;
    }
    sensors->events[sensors->event_count++] =
        (SensorEvent){seconds, kind, sensors->temperature_c};
}

void SensorsReset(Sensors *sensors)
{
    sensors->temperature_c = BASELINE_C;
    sensors->heating = false;
    sensors->impact_count = 0;
    sensors->connected = true;
    sensors->seconds = 0;
    sensors->stale_seconds = 0;
    sensors->event_count = 0;
    Record(sensors, SENSOR_RESET, 0);
}

void SensorsUpdate(Sensors *sensors, float delta_seconds)
{
    if (!isfinite(delta_seconds) || delta_seconds <= 0.0f) return;
    double start = sensors->seconds;
    sensors->seconds += delta_seconds;
    if (!sensors->connected) {
        sensors->stale_seconds += delta_seconds;
        return;
    }
    float previous = sensors->temperature_c;
    bool was_alert = SensorsHeatAlert(sensors);
    float target = sensors->heating ? HEAT_TARGET_C : BASELINE_C;
    float difference = target - sensors->temperature_c;
    float step = CHANGE_C_PER_SECOND * delta_seconds;
    if (fabsf(difference) <= step) sensors->temperature_c = target;
    else sensors->temperature_c += difference > 0.0f ? step : -step;
    if (was_alert != SensorsHeatAlert(sensors)) {
        double crossing = start + fabsf(ALERT_C - previous) / CHANGE_C_PER_SECOND;
        Record(sensors, was_alert ? SENSOR_HEAT_CLEARED : SENSOR_HEAT_RAISED, crossing);
        sensors->events[sensors->event_count - 1].temperature_c = ALERT_C;
    }
}

void SensorsTriggerHeat(Sensors *sensors)
{
    if (!sensors->connected || sensors->heating) return;
    sensors->heating = true;
    Record(sensors, SENSOR_HEAT_STARTED, sensors->seconds);
}

void SensorsStopHeat(Sensors *sensors)
{
    if (!sensors->connected || !sensors->heating) return;
    sensors->heating = false;
    Record(sensors, SENSOR_HEAT_STOPPED, sensors->seconds);
}

void SensorsTriggerImpact(Sensors *sensors)
{
    if (!sensors->connected || sensors->impact_count == UINT_MAX) return;
    ++sensors->impact_count;
    Record(sensors, SENSOR_IMPACT, sensors->seconds);
}

bool SensorsHeatAlert(const Sensors *sensors)
{
    return sensors->temperature_c >= ALERT_C;
}

void SensorsSetConnected(Sensors *sensors, bool connected)
{
    if (sensors->connected == connected) return;
    sensors->connected = connected;
    sensors->stale_seconds = 0;
    Record(sensors, connected ? SENSOR_RECONNECTED : SENSOR_DISCONNECTED, sensors->seconds);
}

const char *SensorsEventName(SensorEventKind kind)
{
    switch (kind) {
        case SENSOR_RESET: return "Monitoring reset";
        case SENSOR_HEAT_STARTED: return "Heat scenario started";
        case SENSOR_HEAT_STOPPED: return "Heat scenario stopped";
        case SENSOR_HEAT_RAISED: return "Heat alert raised";
        case SENSOR_HEAT_CLEARED: return "Heat alert cleared";
        case SENSOR_IMPACT: return "Impact recorded";
        case SENSOR_DISCONNECTED: return "Tracker disconnected";
        case SENSOR_RECONNECTED: return "Tracker reconnected";
    }
    return "Unknown event";
}

bool SensorsWriteCSV(const Sensors *sensors, FILE *output)
{
    if (!output || fprintf(output, "elapsed_seconds,event,temperature_c\n") < 0)
        return false;
    for (unsigned int i = 0; i < sensors->event_count; ++i) {
        const SensorEvent *event = &sensors->events[i];
        if (fprintf(output, "%.3f,\"%s\",%.1f\n", event->seconds,
                    SensorsEventName(event->kind), event->temperature_c) < 0)
            return false;
    }
    return fflush(output) == 0 && !ferror(output);
}

#include "sensors.h"
#include <limits.h>
#include <math.h>

static const float BASELINE_C = 20.0f;
static const float HEAT_TARGET_C = 40.0f;
static const float ALERT_C = 30.0f;
static const float CHANGE_C_PER_SECOND = 2.0f;

void SensorsReset(Sensors *sensors)
{
    sensors->temperature_c = BASELINE_C;
    sensors->heating = false;
    sensors->impact_count = 0;
}

void SensorsUpdate(Sensors *sensors, float delta_seconds)
{
    if (!isfinite(delta_seconds) || delta_seconds <= 0.0f) return;
    float target = sensors->heating ? HEAT_TARGET_C : BASELINE_C;
    float difference = target - sensors->temperature_c;
    float step = CHANGE_C_PER_SECOND * delta_seconds;
    if (fabsf(difference) <= step) sensors->temperature_c = target;
    else sensors->temperature_c += difference > 0.0f ? step : -step;
}

void SensorsTriggerHeat(Sensors *sensors)
{
    sensors->heating = true;
}

void SensorsStopHeat(Sensors *sensors)
{
    sensors->heating = false;
}

void SensorsTriggerImpact(Sensors *sensors)
{
    if (sensors->impact_count < UINT_MAX) ++sensors->impact_count;
}

bool SensorsHeatAlert(const Sensors *sensors)
{
    return sensors->temperature_c >= ALERT_C;
}

#include "../src/sensors.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    Sensors s;
    SensorsReset(&s);
    assert(s.temperature_c == 20.0f && !SensorsHeatAlert(&s));
    assert(s.impact_count == 0);
    SensorsUpdate(&s, 5.0f);
    assert(s.temperature_c == 20.0f);

    SensorsTriggerHeat(&s);
    assert(s.temperature_c == 20.0f); /* Trigger does not jump the reading. */
    SensorsUpdate(&s, 2.0f);
    assert(s.temperature_c == 24.0f && !SensorsHeatAlert(&s));
    SensorsUpdate(&s, 3.0f);
    assert(s.temperature_c == 30.0f && SensorsHeatAlert(&s));
    SensorsUpdate(&s, 100.0f);
    assert(s.temperature_c == 40.0f); /* Clamp to the scenario target. */

    SensorsStopHeat(&s);
    SensorsUpdate(&s, 6.0f);
    assert(s.temperature_c == 28.0f && !SensorsHeatAlert(&s));
    SensorsUpdate(&s, 100.0f);
    assert(s.temperature_c == 20.0f);

    SensorsTriggerImpact(&s);
    assert(s.impact_count == 1);
    SensorsUpdate(&s, 10.0f);
    assert(s.impact_count == 1);
    SensorsTriggerImpact(&s);
    assert(s.impact_count == 2);
    s.impact_count = UINT_MAX;
    SensorsTriggerImpact(&s);
    assert(s.impact_count == UINT_MAX);

    SensorsTriggerHeat(&s);
    SensorsUpdate(&s, 0.0f);
    SensorsUpdate(&s, -1.0f);
    SensorsUpdate(&s, NAN);
    SensorsUpdate(&s, INFINITY);
    assert(s.temperature_c == 20.0f);

    Sensors whole, split;
    SensorsReset(&whole);
    SensorsReset(&split);
    SensorsTriggerHeat(&whole);
    SensorsTriggerHeat(&split);
    SensorsUpdate(&whole, 5.0f);
    for (int i = 0; i < 50; ++i) SensorsUpdate(&split, 0.1f);
    assert(fabsf(whole.temperature_c - split.temperature_c) < 0.001f);

    SensorsReset(&s);
    assert(s.temperature_c == 20.0f && s.impact_count == 0);
    SensorsUpdate(&s, 5.0f);
    assert(s.temperature_c == 20.0f && !SensorsHeatAlert(&s));
    puts("Sensor checks passed: baseline, heat, cooling, impacts, reset and elapsed time.");
    return 0;
}

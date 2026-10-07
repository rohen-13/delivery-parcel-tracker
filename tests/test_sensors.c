#include "../src/sensors.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    Sensors s;
    SensorsReset(&s);
    assert(s.temperature_c == 20.0f && !SensorsHeatAlert(&s));
    FILE *csv = tmpfile();
    assert(csv != NULL);
    SensorsTriggerHeat(&s);
    SensorsUpdate(&s, 20.0f);
    SensorsTriggerImpact(&s);
    assert(SensorsWriteCSV(&s, csv));
    rewind(csv);
    char line[160];
    assert(fgets(line, sizeof(line), csv));
    assert(strcmp(line, "elapsed_seconds,event,temperature_c\n") == 0);
    assert(fgets(line, sizeof(line), csv));
    assert(strcmp(line, "0.000,\"Monitoring reset\",20.0\n") == 0);
    assert(fgets(line, sizeof(line), csv));
    assert(strcmp(line, "0.000,\"Heat scenario started\",20.0\n") == 0);
    assert(fgets(line, sizeof(line), csv));
    assert(strcmp(line, "5.000,\"Heat alert raised\",30.0\n") == 0);
    assert(fgets(line, sizeof(line), csv));
    assert(strcmp(line, "20.000,\"Impact recorded\",40.0\n") == 0);
    assert(fgets(line, sizeof(line), csv) == NULL);
    assert(fclose(csv) == 0);
    assert(!SensorsWriteCSV(&s, NULL));
    FILE *read_only = fopen(__FILE__, "r");
    assert(read_only != NULL);
    assert(!SensorsWriteCSV(&s, read_only));
    assert(fclose(read_only) == 0);
    SensorsReset(&s);
    assert(s.connected && s.event_count == 1);
    SensorsTriggerHeat(&s);
    SensorsUpdate(&s, 5.0f);
    assert(s.events[s.event_count-1].kind == SENSOR_HEAT_RAISED);
    assert(s.events[s.event_count-1].seconds == 5.0);
    SensorsSetConnected(&s, false);
    unsigned int count = s.event_count;
    SensorsSetConnected(&s, false);
    SensorsUpdate(&s, 4.0f);
    SensorsTriggerImpact(&s);
    SensorsStopHeat(&s);
    assert(s.temperature_c == 30.0f && s.stale_seconds == 4.0);
    assert(s.impact_count == 0 && s.event_count == count && s.heating);
    SensorsSetConnected(&s, true);
    assert(s.stale_seconds == 0.0 && s.connected);
    SensorsStopHeat(&s);
    SensorsUpdate(&s, 1.0f);
    assert(s.events[s.event_count-1].kind == SENSOR_HEAT_CLEARED);
    for (int i = 0; i < 50; ++i) SensorsTriggerImpact(&s);
    assert(s.event_count == SENSOR_EVENT_CAPACITY && s.impact_count == 50);
    csv = tmpfile();
    assert(csv && SensorsWriteCSV(&s, csv));
    rewind(csv);
    unsigned int rows = 0;
    while (fgets(line, sizeof(line), csv)) ++rows;
    assert(rows == SENSOR_EVENT_CAPACITY + 1);
    assert(fclose(csv) == 0);
    for (unsigned int i = 1; i < s.event_count; ++i)
        assert(s.events[i].seconds >= s.events[i-1].seconds);
    SensorsReset(&s);
    assert(s.connected && s.stale_seconds == 0 && s.impact_count == 0);
    assert(s.event_count == 1 && s.events[0].seconds == 0);
    SensorsTriggerHeat(&s);
    SensorsUpdate(&s, 20.0f);
    assert(s.events[s.event_count-1].seconds == 5.0);
    assert(s.events[s.event_count-1].temperature_c == 30.0f);
    SensorsReset(&s);
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

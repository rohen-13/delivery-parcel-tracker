#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>
#include <stdio.h>

#define SENSOR_EVENT_CAPACITY 32
typedef enum {
    SENSOR_RESET, SENSOR_HEAT_STARTED, SENSOR_HEAT_STOPPED,
    SENSOR_HEAT_RAISED, SENSOR_HEAT_CLEARED, SENSOR_IMPACT,
    SENSOR_DISCONNECTED, SENSOR_RECONNECTED
} SensorEventKind;
typedef struct {
    double seconds;
    SensorEventKind kind;
    float temperature_c;
    unsigned int impact_count;
} SensorEvent;

/* Demonstration values in Celsius, not shipping safety limits. */
typedef struct {
    float temperature_c;
    bool heating;
    unsigned int impact_count;
    bool connected;
    double seconds;
    double stale_seconds;
    float peak_temperature_c;
    double heat_seconds; /* Observed time at or above the demo threshold. */
    double unobserved_seconds; /* Lifetime disconnected time since reset. */
    SensorEvent events[SENSOR_EVENT_CAPACITY];
    unsigned int event_count;
} Sensors;

void SensorsReset(Sensors *sensors);
/* Call with elapsed seconds independently of parcel movement. */
void SensorsUpdate(Sensors *sensors, float delta_seconds);
void SensorsTriggerHeat(Sensors *sensors);
void SensorsStopHeat(Sensors *sensors);
/* Each call records one impact; updates never repeat it. */
void SensorsTriggerImpact(Sensors *sensors);
bool SensorsHeatAlert(const Sensors *sensors);
void SensorsSetConnected(Sensors *sensors, bool connected);
const char *SensorsEventName(SensorEventKind kind);
/* Writes retained events, oldest first; does not close the caller's stream. */
bool SensorsWriteCSV(const Sensors *sensors, FILE *output);
/* Exclusively creates the next numbered report in directory. On failure,
 * name is empty; a partial file may remain after a write/close error. */
bool SensorsExportCSV(const Sensors *sensors, const char *directory,
                      char *name, size_t name_size);

#endif

#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>

/* Demonstration values in Celsius, not shipping safety limits. */
typedef struct {
    float temperature_c;
    bool heating;
    unsigned int impact_count;
} Sensors;

void SensorsReset(Sensors *sensors);
/* Call with elapsed seconds independently of parcel movement. */
void SensorsUpdate(Sensors *sensors, float delta_seconds);
void SensorsTriggerHeat(Sensors *sensors);
void SensorsStopHeat(Sensors *sensors);
/* Each call records one impact; updates never repeat it. */
void SensorsTriggerImpact(Sensors *sensors);
bool SensorsHeatAlert(const Sensors *sensors);

#endif

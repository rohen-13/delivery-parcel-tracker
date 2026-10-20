#ifndef TEMPERATURE_HISTORY_H
#define TEMPERATURE_HISTORY_H

#include <stdbool.h>

#define TEMPERATURE_HISTORY_CAPACITY 128
#define TEMPERATURE_HISTORY_SECONDS 60.0

typedef struct {
    double seconds;
    float temperature_c;
    bool fresh;
} TemperatureSample;

typedef struct {
    TemperatureSample samples[TEMPERATURE_HISTORY_CAPACITY];
    unsigned int count;
} TemperatureHistory;

void TemperatureHistoryReset(TemperatureHistory *history);
/* Observe actual telemetry once per frame; retain at most one regular sample
 * per second plus connection transitions. Never interpolate missing readings. */
void TemperatureHistoryObserve(TemperatureHistory *history, double seconds,
                               float temperature_c, bool connected);
bool TemperatureHistoryCanJoin(const TemperatureSample *a, const TemperatureSample *b);

#endif

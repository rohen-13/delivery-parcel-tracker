#include "temperature_history.h"
#include <math.h>
#include <string.h>

void TemperatureHistoryReset(TemperatureHistory *history)
{
    history->count = 0;
}

void TemperatureHistoryObserve(TemperatureHistory *history, double seconds,
                               float temperature_c, bool connected)
{
    if (!isfinite(seconds) || seconds < 0 || !isfinite(temperature_c)) return;
    if (history->count && seconds < history->samples[history->count-1].seconds)
        TemperatureHistoryReset(history);
    unsigned int expired = 0;
    while (expired < history->count &&
           seconds-history->samples[expired].seconds > TEMPERATURE_HISTORY_SECONDS)
        ++expired;
    if (expired) {
        history->count -= expired;
        memmove(history->samples, history->samples+expired,
                history->count*sizeof(history->samples[0]));
    }
    TemperatureSample sample = {seconds, temperature_c, connected};
    if (history->count) {
        TemperatureSample *last = &history->samples[history->count-1];
        if (seconds == last->seconds) { *last = sample; return; }
        if (connected == last->fresh && seconds-last->seconds < 1.0) return;
    }
    if (history->count == TEMPERATURE_HISTORY_CAPACITY) {
        memmove(history->samples, history->samples+1,
                (TEMPERATURE_HISTORY_CAPACITY-1)*sizeof(history->samples[0]));
        --history->count;
    }
    history->samples[history->count++] = sample;
}

bool TemperatureHistoryCanJoin(const TemperatureSample *a, const TemperatureSample *b)
{
    double gap = b->seconds-a->seconds;
    return a->fresh && b->fresh && gap > 0 && gap <= 2.0;
}

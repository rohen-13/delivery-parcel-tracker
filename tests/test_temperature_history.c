#include "../src/temperature_history.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    TemperatureHistory h;
    TemperatureHistoryReset(&h);
    TemperatureHistoryObserve(&h, 0, 20, true);
    TemperatureHistoryObserve(&h, 0.5, 21, true);
    assert(h.count == 1); /* Rendering frames do not fill the history. */
    TemperatureHistoryObserve(&h, 1, 22, true);
    assert(h.count == 2 && h.samples[1].temperature_c == 22);
    assert(TemperatureHistoryCanJoin(&h.samples[0], &h.samples[1]));
    TemperatureHistoryObserve(&h, 1.1, 22, false);
    TemperatureHistoryObserve(&h, 1.2, 22, true);
    assert(h.count == 4); /* Even a brief disconnect must break the line. */
    assert(!TemperatureHistoryCanJoin(&h.samples[1], &h.samples[2]));
    assert(!TemperatureHistoryCanJoin(&h.samples[2], &h.samples[3]));
    TemperatureHistoryObserve(&h, 10, 35, true);
    assert(!TemperatureHistoryCanJoin(&h.samples[3], &h.samples[4]));
    TemperatureHistoryObserve(&h, NAN, 20, true);
    TemperatureHistoryObserve(&h, 11, INFINITY, true);
    assert(h.count == 5);
    TemperatureHistoryObserve(&h, 10, 36, true);
    assert(h.count == 5 && h.samples[4].temperature_c == 36);
    TemperatureHistoryObserve(&h, 0, 20, true);
    assert(h.count == 1 && h.samples[0].seconds == 0);
    for (int i=1; i<=100; ++i) TemperatureHistoryObserve(&h, i, 20+i/10.0f, true);
    assert(h.count == 61 && h.samples[0].seconds == 40);
    TemperatureHistoryReset(&h);
    for (int i=0; i<200; ++i)
        TemperatureHistoryObserve(&h, i/10.0, 20, i%2 == 0);
    assert(h.count == TEMPERATURE_HISTORY_CAPACITY);
    assert(h.samples[h.count-1].seconds == 19.9);
    TemperatureHistoryReset(&h);
    assert(h.count == 0);
    puts("Temperature history checks passed: sampling, gaps, reconnect, reset, bounds and capacity.");
    return 0;
}

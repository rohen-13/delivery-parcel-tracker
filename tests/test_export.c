#include "../src/sensors.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    assert(argc == 2);
    Sensors s;
    SensorsReset(&s);
    SensorsTriggerImpact(&s);
    char name[32], path[4096], line[160];
    assert(SensorsExportCSV(&s, argv[1], name, sizeof(name)));
    assert(strcmp(name, "events-001.csv") == 0);
    SensorsReset(&s);
    assert(SensorsExportCSV(&s, argv[1], name, sizeof(name)));
    assert(strcmp(name, "events-002.csv") == 0);
    snprintf(path, sizeof(path), "%s/events-001.csv", argv[1]);
    FILE *file = fopen(path, "r");
    assert(file);
    assert(fgets(line, sizeof(line), file));
    assert(strcmp(line, "elapsed_seconds,event,temperature_c,impact_count\n") == 0);
    assert(fgets(line, sizeof(line), file));
    assert(strcmp(line, "0.000,\"Monitoring reset\",20.0,0\n") == 0);
    assert(fgets(line, sizeof(line), file));
    assert(strcmp(line, "0.000,\"Impact recorded\",20.0,1\n") == 0);
    assert(!fgets(line, sizeof(line), file));
    assert(fclose(file) == 0);
    snprintf(path, sizeof(path), "%s/events-002.csv", argv[1]);
    file = fopen(path, "r");
    assert(file);
    assert(fgets(line, sizeof(line), file));
    assert(fgets(line, sizeof(line), file));
    assert(!fgets(line, sizeof(line), file));
    assert(fclose(file) == 0);
    assert(s.event_count == 1);
    name[0] = 'x';
    assert(!SensorsExportCSV(&s, path, name, sizeof(name)));
    assert(name[0] == '\0');
    assert(!SensorsExportCSV(&s, argv[1], name, 2));
    assert(!SensorsExportCSV(&s, NULL, name, sizeof(name)));
    assert(!SensorsExportCSV(&s, argv[1], NULL, 0));
    /* The UI directory includes a trailing slash; both forms must agree. */
    snprintf(path, sizeof(path), "%s/", argv[1]);
    assert(SensorsExportCSV(&s, path, name, sizeof(name)));
    assert(strcmp(name, "events-003.csv") == 0);
    for (unsigned int i = 4; i <= 999; ++i) {
        snprintf(path, sizeof(path), "%s/events-%03u.csv", argv[1], i);
        file = fopen(path, "wx");
        assert(file && fputs("preserve\n", file) >= 0);
        assert(fclose(file) == 0);
    }
    assert(!SensorsExportCSV(&s, argv[1], name, sizeof(name)));
    assert(name[0] == '\0');
    file = fopen(path, "r");
    assert(file && fgets(line, sizeof(line), file));
    assert(strcmp(line, "preserve\n") == 0);
    assert(fclose(file) == 0);
    puts("Export checks passed: repeated saves, reset preservation and failures.");
}

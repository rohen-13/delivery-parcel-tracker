#ifndef ROUND_H
#define ROUND_H
#include "dispatch.h"
#include "sensors.h"
#include "temperature_history.h"

typedef struct {
    Package packages[DISPATCH_MAX_PACKAGES];
    Sensors sensors[DISPATCH_MAX_PACKAGES];
    TemperatureHistory histories[DISPATCH_MAX_PACKAGES];
    DeliveryRoute route;
    GeoPoint position;
    int selected;
} DeliveryRound;

void RoundReset(DeliveryRound *round);
enum { ROUND_ISSUE_CONNECTION=1, ROUND_ISSUE_HEAT=2, ROUND_ISSUE_IMPACT=4 };
unsigned RoundPackageIssues(const DeliveryRound *round,int index);
int RoundIssueCount(const DeliveryRound *round);
bool RoundSelectNextIssue(DeliveryRound *round);
void RoundUpdate(DeliveryRound *round, float seconds);
bool RoundSelect(DeliveryRound *round, int index);
bool RoundSetIncluded(DeliveryRound *round, int index, bool included);
bool RoundDeliverNext(DeliveryRound *round);
bool RoundWriteCSV(const DeliveryRound *round, FILE *output);
bool RoundWriteSummaryCSV(const DeliveryRound *round,FILE *output);
bool RoundExportSummaryCSV(const DeliveryRound *round,const char *directory,char *name,size_t capacity);
bool RoundExportCSV(const DeliveryRound *round,const char *directory,char *name,size_t capacity);
#endif

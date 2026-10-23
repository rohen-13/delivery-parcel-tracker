#ifndef ROUND_H
#define ROUND_H
#include "dispatch.h"
#include "sensors.h"
#include "temperature_history.h"
#define ROUND_RETAINED_ATTEMPTS 8
typedef enum { ATTEMPT_DELIVERED, ATTEMPT_RECIPIENT_ABSENT, ATTEMPT_NO_ACCESS, ATTEMPT_ADDRESS_PROBLEM } AttemptOutcome;
typedef struct { double seconds; AttemptOutcome outcome; } DeliveryAttempt;
typedef enum { DRIVE_READY, DRIVE_RUNNING, DRIVE_PAUSED, DRIVE_ARRIVED } DriveState;
typedef struct {
    DriveState state;
    int speed;
    double elapsed_seconds, distance_m, total_m;
} DeliveryRun;

typedef struct {
    Package packages[DISPATCH_MAX_PACKAGES];
    Sensors sensors[DISPATCH_MAX_PACKAGES];
    TemperatureHistory histories[DISPATCH_MAX_PACKAGES];
    DeliveryRoute route;
    GeoPoint position;
    int selected;
    DeliveryRun run;
    bool deferred[DISPATCH_MAX_PACKAGES];
    unsigned attempt_count[DISPATCH_MAX_PACKAGES];
    DeliveryAttempt attempts[DISPATCH_MAX_PACKAGES][ROUND_RETAINED_ATTEMPTS];
} DeliveryRound;

void RoundReset(DeliveryRound *round);
bool RoundDriveStart(DeliveryRound *round);
void RoundDrivePause(DeliveryRound *round);
bool RoundDriveSpeed(DeliveryRound *round,int multiplier);
bool RoundCanAttempt(const DeliveryRound *round);
enum { ROUND_ISSUE_CONNECTION=1, ROUND_ISSUE_HEAT=2, ROUND_ISSUE_IMPACT=4, ROUND_ISSUE_DELIVERY=8 };
const char *RoundAttemptName(AttemptOutcome outcome);
bool RoundFailNext(DeliveryRound *round,AttemptOutcome outcome);
bool RoundRetry(DeliveryRound *round,int index);
bool RoundWriteAttemptsCSV(const DeliveryRound *round,FILE *output);
bool RoundExportAttemptsCSV(const DeliveryRound *round,const char *directory,char *name,size_t capacity);
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

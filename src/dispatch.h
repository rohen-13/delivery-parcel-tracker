#ifndef DISPATCH_H
#define DISPATCH_H
#include <stdbool.h>
#include <stddef.h>
#define DISPATCH_MAX_PACKAGES 5
#define DISPATCH_MAX_POINTS 8192
typedef struct { double lat, lon; } GeoPoint;
typedef struct {
    char id[20], destination[48];
    GeoPoint point;
    bool included, delivered;
} Package;
typedef struct {
    int order[DISPATCH_MAX_PACKAGES], count;
    double distance_km, duration_seconds;
    GeoPoint points[DISPATCH_MAX_POINTS];
    int point_count;
    bool ready;
    char message[160];
} DeliveryRoute;
/* These are public demo credentials, never real account authentication. */
bool DispatchLogin(const char *user, const char *password);
void DispatchSeed(Package packages[DISPATCH_MAX_PACKAGES]);
bool DispatchBestOrder(const double distances[6][6], int count, int order[5], double *km);
bool DispatchParseMatrix(const char *json, int count, double distances[6][6]);
bool DispatchParseRoute(const char *json, DeliveryRoute *route);
bool DispatchPlan(const Package packages[5], DeliveryRoute *route);
/* Existing system curl, direct argv execution (no shell), bounded HTTPS fetch. */
bool DispatchFetch(const char *url, const char *json, unsigned char *data, size_t capacity, size_t *size);
extern const GeoPoint DISPATCH_DEPOT;
#endif

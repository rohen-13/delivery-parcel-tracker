#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "tracker.h"
#include "sensors.h"
#include "raylib.h"

void DashboardDraw(Tracker *tracker, Sensors *sensors);
void DashboardInit(void);
void DashboardClose(void);
bool DashboardSignedIn(void);
bool DashboardAccountPage(void);
void DashboardOpenProfile(void);
void DashboardObserveSensors(const Sensors *sensors);
void DashboardSetViewport(float scale, Vector2 offset);

#endif

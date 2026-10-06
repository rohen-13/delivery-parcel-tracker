#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "tracker.h"
#include "sensors.h"
#include "raylib.h"

void DashboardDraw(Tracker *tracker, Sensors *sensors);
void DashboardInit(void);
void DashboardClose(void);

#endif

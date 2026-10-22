#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "round.h"
#include "sensors.h"
#include "raylib.h"

void DashboardDraw(DeliveryRound *round);
/* Actions: 0 stay, 1 dispatch, 2 profile, 3 lock. */
int DashboardDrawMonitoring(DeliveryRound *round);
void DashboardClearExportStatus(void);
void DashboardInit(void);
void DashboardClose(void);
bool DashboardSignedIn(void);
bool DashboardAccountPage(void);
void DashboardOpenProfile(void);
void DashboardLock(void);
void DashboardSetViewport(float scale, Vector2 offset);

#endif

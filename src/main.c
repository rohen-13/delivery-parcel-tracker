#include "raylib.h"
#include "dashboard.h"
#include "tracker.h"

int main(void)
{
    Tracker tracker;
    TrackerReset(&tracker);
    InitWindow(1100, 700, "Delivery Parcel Tracker");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        TrackerUpdate(&tracker, GetFrameTime());
        BeginDrawing();
        DashboardDraw(&tracker);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}

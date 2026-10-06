#include "raylib.h"
#include "dashboard.h"
#include "tracker.h"

int main(void)
{
    Tracker tracker;
    Sensors sensors;
    TrackerReset(&tracker);
    SensorsReset(&sensors);
    InitWindow(1100, 840, "Delivery Parcel Tracker");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        float delta_seconds = GetFrameTime();
        TrackerUpdate(&tracker, delta_seconds);
        SensorsUpdate(&sensors, delta_seconds);
        BeginDrawing();
        DashboardDraw(&tracker, &sensors);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}

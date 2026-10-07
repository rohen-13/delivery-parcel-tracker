#include "raylib.h"
#include "dashboard.h"
#include "tracker.h"

int main(void)
{
    Tracker tracker;
    Sensors sensors;
    TrackerReset(&tracker);
    SensorsReset(&sensors);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1280, 1040, "Delivery Parcel Tracker");
    DashboardInit();
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        float delta_seconds = GetFrameTime();
        TrackerUpdate(&tracker, delta_seconds);
        SensorsUpdate(&sensors, delta_seconds);
        BeginDrawing();
        DashboardDraw(&tracker, &sensors);
        EndDrawing();
    }
    DashboardClose();
    CloseWindow();
    return 0;
}

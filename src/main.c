#include "raylib.h"
#include "dashboard.h"
#include "tracker.h"
#include "dispatch_ui.h"

int main(void)
{
    Tracker tracker;
    Sensors sensors;
    TrackerReset(&tracker);
    SensorsReset(&sensors);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1280, 1040, "Delivery Parcel Tracker");
    DashboardInit();
    DispatchUIInit();
    bool monitoring=false;
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        float delta_seconds = GetFrameTime();
        TrackerUpdate(&tracker, delta_seconds);
        SensorsUpdate(&sensors, delta_seconds);
        BeginDrawing();
        if(!monitoring) monitoring=DispatchUIDraw()!=0;
        else {
            DashboardDraw(&tracker, &sensors);
            Rectangle back={28,900,224,40};
            DrawRectangleRec(back,(Color){38,53,43,255});
            DrawText("Dispatch map (M)",42,912,17,WHITE);
            if(IsKeyPressed(KEY_M) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
               CheckCollisionPointRec(GetMousePosition(),back)))monitoring=false;
        }
        EndDrawing();
    }
    DashboardClose();
    DispatchUIClose();
    CloseWindow();
    return 0;
}

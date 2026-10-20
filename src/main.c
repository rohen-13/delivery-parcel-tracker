#include "raylib.h"
#include "dashboard.h"
#include "tracker.h"
#include "dispatch_ui.h"
#include <math.h>

int main(void)
{
    Tracker tracker;
    Sensors sensors;
    TrackerReset(&tracker);
    SensorsReset(&sensors);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1024, 832, "Delivery Parcel Tracker");
    SetWindowMinSize(640, 520);
    DashboardInit();
    DispatchUIInit();
    bool monitoring=false;
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        float delta_seconds = GetFrameTime();
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        if (DashboardSignedIn()) {
            TrackerUpdate(&tracker, delta_seconds);
            SensorsUpdate(&sensors, delta_seconds);
            DashboardObserveSensors(&sensors);
        }
        float scale = fminf(GetScreenWidth()/1280.0f, GetScreenHeight()/1040.0f);
        Vector2 offset = {(GetScreenWidth()-1280*scale)/2,
                          (GetScreenHeight()-1040*scale)/2};
        SetMouseOffset(-(int)offset.x, -(int)offset.y);
        SetMouseScale(1/scale, 1/scale);
        DashboardSetViewport(scale, offset);
        DispatchUISetViewport(scale, offset);
        BeginDrawing();
        ClearBackground((Color){12,17,23,255});
        BeginMode2D((Camera2D){.offset=offset, .zoom=scale});
        bool signed_in = DashboardSignedIn();
        if (DashboardAccountPage()) DashboardDraw(&tracker, &sensors);
        else if (!monitoring) {
            int action = DispatchUIDraw();
            if (action == 1) monitoring = true;
            if (action == 2) DashboardOpenProfile();
        } else {
            DashboardDraw(&tracker, &sensors);
            Rectangle back={28,946,224,40};
            DrawRectangleRec(back,(Color){38,53,43,255});
            DrawText("Dispatch map (M)",42,958,17,WHITE);
            if(IsKeyPressed(KEY_M) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
               CheckCollisionPointRec(GetMousePosition(),back)))monitoring=false;
        }
        if (signed_in && !DashboardSignedIn()) {
            DispatchUIReset();
            monitoring = false;
        }
        EndMode2D();
        EndDrawing();
    }
    DashboardClose();
    DispatchUIClose();
    CloseWindow();
    return 0;
}

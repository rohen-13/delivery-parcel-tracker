#include "raylib.h"
#include "dashboard.h"
#include "dispatch_ui.h"
#include <math.h>

int main(void)
{
    static DeliveryRound round;
    bool monitoring = false;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1024, 832, "Delivery Parcel Tracker");
    SetWindowMinSize(640, 520);
    DashboardInit();
    DispatchUIInit(&round);
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        float delta_seconds = GetFrameTime();
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        if (DashboardSignedIn()) {
            if(DashboardAccountPage())RoundDrivePause(&round);
            RoundUpdate(&round,delta_seconds);
        }
        DispatchUIUpdate();
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
        if (DashboardAccountPage()) DashboardDraw(&round);
        else {
            int action = monitoring ? DashboardDrawMonitoring(&round) : DispatchUIDraw();
            if (action == 1) monitoring = !monitoring;
            if (action == 2) DashboardOpenProfile();
            if (action == 3) DashboardLock();
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

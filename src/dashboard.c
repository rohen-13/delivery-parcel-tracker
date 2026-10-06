#include "dashboard.h"
#include <math.h>

static const Color BG = {16, 23, 38, 255};
static const Color PANEL = {27, 38, 57, 255};
static const Color MUTED = {156, 173, 197, 255};
static const Color ACCENT = {65, 211, 171, 255};
static const Color WARNING = {255, 190, 100, 255};

static bool Button(Rectangle bounds, const char *label, bool enabled)
{
    bool hover = CheckCollisionPointRec(GetMousePosition(), bounds);
    Color fill = enabled ? (hover ? ACCENT : (Color){48, 89, 102, 255}) : PANEL;
    DrawRectangleRounded(bounds, 0.2f, 8, fill);
    int size = 20;
    DrawText(label, (int)(bounds.x + (bounds.width - MeasureText(label, size))/2),
             (int)(bounds.y + (bounds.height - size)/2), size, enabled ? RAYWHITE : MUTED);
    return enabled && hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void DashboardDraw(Tracker *tracker, Sensors *sensors)
{
    const Vector2 route[] = {{110, 370}, {350, 290}, {610, 390}, {850, 300}};
    const char *labels[] = {"Depot", "Checkpoint 1", "Checkpoint 2", "Destination"};
    float lengths[3], total = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float dx = route[i+1].x - route[i].x, dy = route[i+1].y - route[i].y;
        lengths[i] = sqrtf(dx*dx + dy*dy);
        total += lengths[i];
    }
    float progress = TrackerProgress(tracker);
    float distance = progress * total;
    Vector2 parcel = route[3];
    for (int i = 0; i < 3; ++i) {
        if (distance <= lengths[i]) {
            float t = distance / lengths[i];
            parcel = (Vector2){route[i].x + (route[i+1].x-route[i].x)*t,
                               route[i].y + (route[i+1].y-route[i].y)*t};
            break;
        }
        distance -= lengths[i];
    }
    const char *status = TrackerStatus(tracker);
        ClearBackground(BG);
        DrawText("DELIVERY PARCEL TRACKER", 42, 35, 30, RAYWHITE);
        DrawText("Track a parcel from dispatch to delivery", 43, 78, 19, MUTED);
        DrawRectangleRounded((Rectangle){880, 37, 180, 38}, 0.3f, 8, PANEL);
        DrawText("SIMULATION MODE", 898, 49, 16, ACCENT);
        DrawRectangleRounded((Rectangle){40, 120, 1020, 90}, 0.15f, 8, PANEL);
        DrawText("PARCEL", 64, 138, 15, MUTED);
        DrawText("PKG-001", 64, 163, 24, RAYWHITE);
        DrawText("STATUS", 390, 138, 15, MUTED);
        DrawText(status, 390, 163, 24, ACCENT);
        DrawText("DELIVERY PROGRESS", 730, 138, 15, MUTED);
        DrawText(TextFormat("%.0f%%", progress * 100.0f), 730, 163, 24, RAYWHITE);
        DrawRectangleRounded((Rectangle){40, 235, 1020, 250}, 0.06f, 8, PANEL);
        for (int i = 0; i < 3; ++i) DrawLineEx(route[i], route[i+1], 6, (Color){78, 96, 121, 255});
        for (int i = 0; i < 4; ++i) {
            DrawCircleV(route[i], 10, MUTED);
            DrawText(labels[i], (int)route[i].x - MeasureText(labels[i], 18)/2,
                     (int)route[i].y + 30, 18, RAYWHITE);
        }
        DrawCircleV(parcel, 25, BG);
        DrawRectangleRounded((Rectangle){parcel.x-16, parcel.y-16, 32, 32}, 0.15f, 6, ACCENT);
        DrawLine((int)parcel.x, (int)parcel.y-16, (int)parcel.x, (int)parcel.y+2, BG);
        DrawText("PKG-001", (int)parcel.x - 34, (int)parcel.y - 48, 16, ACCENT);
        DrawRectangleRounded((Rectangle){40, 515, 1020, 12}, 0.4f, 6, PANEL);
        if (progress > 0.0f) DrawRectangleRounded((Rectangle){40, 515, 1020*progress, 12}, 0.4f, 6, ACCENT);
        if (Button((Rectangle){40, 558, 175, 52}, tracker->elapsed_seconds > 0.0f ? "Resume" : "Start", !tracker->running && progress < 1.0f)) TrackerStart(tracker);
        if (Button((Rectangle){235, 558, 175, 52}, "Pause", tracker->running)) TrackerPause(tracker);
        if (Button((Rectangle){430, 558, 175, 52}, "Reset", true)) {
            TrackerReset(tracker);
            SensorsReset(sensors);
        }
        DrawRectangleRounded((Rectangle){40, 635, 1020, 90}, 0.15f, 8, PANEL);
        DrawText("TEMPERATURE", 64, 651, 15, MUTED);
        DrawText(TextFormat("%.1f C", sensors->temperature_c), 64, 677, 24, RAYWHITE);
        bool heat_alert = SensorsHeatAlert(sensors);
        DrawText(heat_alert ? "HEAT ALERT - 30 C or above" : "Temperature below demo threshold",
                 260, 677, 18, heat_alert ? WARNING : ACCENT);
        DrawText("IMPACT EVENTS", 790, 651, 15, MUTED);
        DrawText(TextFormat("%u", sensors->impact_count), 790, 677, 24,
                 sensors->impact_count > 0 ? WARNING : RAYWHITE);
        if (Button((Rectangle){40, 741, 175, 52}, "Trigger heat", !sensors->heating)) SensorsTriggerHeat(sensors);
        if (Button((Rectangle){235, 741, 175, 52}, "Stop heat", sensors->heating)) SensorsStopHeat(sensors);
        if (Button((Rectangle){430, 741, 175, 52}, "Trigger impact", true)) SensorsTriggerImpact(sensors);
        DrawText("Simulated sensors stay active while paused | Demo values only", 42, 810, 17, MUTED);

}

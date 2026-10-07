#include "dashboard.h"
#include <math.h>
#include <stddef.h>

static const Color BG = {12,17,23,255}, SURFACE = {20,28,36,255};
static const Color INK = {235,240,246,255}, MUTED = {153,170,186,255};
static const Color BORDER = {43,57,69,255}, ACCENT = {203,239,105,255};
static const Color GOOD = {121,216,177,255}, AMBER = {255,185,112,255};
static Font font;
static bool custom_font;

void DashboardInit(void)
{
    const char *path = "/System/Library/Fonts/Supplemental/Arial.ttf";
    custom_font = FileExists(path);
    font = custom_font ? LoadFontEx(path, 64, NULL, 0) : GetFontDefault();
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
}
void DashboardClose(void)
{
    if (custom_font) UnloadFont(font);
}
static void Text(const char *value, float x, float y, float size, Color color)
{
    DrawTextEx(font, value, (Vector2){x,y}, size, 0, color);
}
static void Card(Rectangle r)
{
    DrawRectangleRounded(r, 0.07f, 10, SURFACE);
    DrawRectangleRoundedLinesEx(r, 0.07f, 10, 1, BORDER);
}
static bool Button(Rectangle r, const char *label, bool enabled, bool primary)
{
    bool hover = enabled && CheckCollisionPointRec(GetMousePosition(),r);
    Color fill = primary ? ACCENT : SURFACE;
    if (hover) fill = primary ? (Color){222,252,143,255} : (Color){34,47,59,255};
    if (!enabled) fill = (Color){26,35,43,255};
    DrawRectangleRounded(r, 0.18f, 10, fill);
    if (!primary) DrawRectangleRoundedLinesEx(r, 0.18f, 10, 1, BORDER);
    float width = MeasureTextEx(font,label,16,0).x;
    Text(label,r.x+(r.width-width)/2,r.y+13,16,!enabled ? MUTED : primary ? BG : INK);
    if (hover) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
void DashboardDraw(Tracker *tracker, Sensors *sensors)
{
    float progress = TrackerProgress(tracker);
    bool alert = SensorsHeatAlert(sensors);
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    ClearBackground(BG);

    /* Shipment rail: actual state and controls, with no decorative navigation. */
    DrawRectangle(0,0,280,820,SURFACE);
    DrawLine(280,0,280,820,BORDER);
    DrawRectangleRounded((Rectangle){28,28,34,34},0.2f,8,ACCENT);
    DrawRectangleLinesEx((Rectangle){37,36,16,18},2,BG);
    DrawLine(45,36,45,45,BG);
    Text("PARCEL / OPS",76,35,19,INK);
    Text("SHIPMENT CONSOLE",28,102,11,MUTED);
    Text("PKG-001",28,126,32,INK);
    Text("Single parcel demonstration",28,170,14,MUTED);
    DrawLine(28,207,252,207,BORDER);
    Text("DELIVERY STATE",28,232,11,MUTED);
    DrawCircle(34,271,5,tracker->running ? ACCENT : MUTED);
    Text(TrackerStatus(tracker),49,257,24,INK);
    Text("ROUTE PROGRESS",28,317,11,MUTED);
    Text(TextFormat("%.0f",progress*100),28,342,56,INK);
    Text("%",113,369,21,MUTED);
    DrawRectangleRounded((Rectangle){28,416,224,6},1,8,BORDER);
    if (progress>0) DrawRectangleRounded((Rectangle){28,416,224*progress,6},1,8,ACCENT);
    Text("20 second simulated journey",28,440,13,MUTED);
    DrawLine(28,484,252,484,BORDER);
    Text("DELIVERY CONTROLS",28,511,11,MUTED);
    if (Button((Rectangle){28,542,224,44},tracker->elapsed_seconds>0 ? "Resume delivery" : "Start delivery",
               !tracker->running && progress<1,true)) TrackerStart(tracker);
    if (Button((Rectangle){28,598,104,44},"Pause",tracker->running,false)) TrackerPause(tracker);
    if (Button((Rectangle){144,598,108,44},"Reset all",true,false)) {
        TrackerReset(tracker);
        SensorsReset(sensors);
    }
    Text("C + RAYLIB",28,731,12,MUTED);
    Text("Advanced Programming",28,756,14,INK);
    Text("Prototype / simulation",28,781,12,MUTED);

    Text("Delivery overview",308,28,30,INK);
    Text("Follow the shipment. Inspect its handling conditions.",309,68,15,MUTED);
    DrawRectangleRounded((Rectangle){1057,33,195,32},0.4f,8,(Color){31,45,34,255});
    DrawCircle(1074,49,4,ACCENT);
    Text("SIMULATION ACTIVE",1088,41,12,ACCENT);

    /* Fictional street schematic; this is not a real geographic map. */
    Rectangle map={308,112,944,392};
    Card(map);
    Text("ROUTE MONITOR",332,135,12,MUTED);
    Text("Depot to destination",332,158,20,INK);
    Text("Fictional route / no GPS connected",992,141,12,MUTED);
    Color grid={29,39,48,255}, street={38,50,61,255};
    for (int row=0;row<4;++row) {
        for (int col=0;col<9;++col) {
            float x=332+col*99, y=211+row*56;
            DrawRectangleRounded((Rectangle){x,y,78,36},0.12f,6,grid);
        }
    }
    for (int col=0;col<8;++col)
        DrawLineEx((Vector2){420+col*99,207},(Vector2){420+col*99,446},1,street);
    for (int row=0;row<4;++row)
        DrawLineEx((Vector2){331,253+row*56},(Vector2){1227,253+row*56},1,street);
    const Vector2 route[]={{366,376},{620,286},{900,370},{1192,270}};
    const char *names[]={"DEPOT","HUB 01","HUB 02","DESTINATION"};
    float lengths[3],total=0;
    for (int i=0;i<3;++i) {
        float dx=route[i+1].x-route[i].x,dy=route[i+1].y-route[i].y;
        lengths[i]=sqrtf(dx*dx+dy*dy);
        total+=lengths[i];
        DrawLineEx(route[i],route[i+1],5,(Color){71,89,104,255});
    }
    float remaining=progress*total;
    Vector2 parcel=route[0];
    for (int i=0;i<3;++i) {
        float travelled=fminf(fmaxf(remaining,0),lengths[i]);
        float t=travelled/lengths[i];
        Vector2 end={route[i].x+(route[i+1].x-route[i].x)*t,
                     route[i].y+(route[i+1].y-route[i].y)*t};
        if (travelled>0) DrawLineEx(route[i],end,5,ACCENT);
        if (remaining>=0) parcel=end;
        remaining-=lengths[i];
    }
    float checkpoint_distance=0;
    for (int i=0;i<4;++i) {
        if (i>0) checkpoint_distance+=lengths[i-1];
        bool reached=progress*total>=checkpoint_distance;
        DrawCircleV(route[i],9,SURFACE);
        DrawCircleLinesV(route[i],9,reached ? ACCENT : MUTED);
        DrawCircleV(route[i],3,reached ? ACCENT : MUTED);
        float width=MeasureTextEx(font,names[i],11,0).x;
        DrawRectangleRounded((Rectangle){route[i].x-width/2-8,route[i].y+20,width+16,25},0.2f,8,BG);
        Text(names[i],route[i].x-width/2,route[i].y+27,11,INK);
    }
    DrawCircleV(parcel,20,(Color){47,65,42,255});
    DrawCircleV(parcel,12,ACCENT);
    DrawRectangleLinesEx((Rectangle){parcel.x-5,parcel.y-6,10,12},1.5f,BG);
    DrawLineEx((Vector2){parcel.x,parcel.y-6},(Vector2){parcel.x,parcel.y},1.5f,BG);
    DrawLine(332,458,1228,458,BORDER);
    DrawCircle(339,480,4,ACCENT);
    Text("Completed route",351,472,12,MUTED);
    DrawCircle(491,480,4,(Color){71,89,104,255});
    Text("Remaining route",503,472,12,MUTED);
    Text("Position derived from delivery progress",987,472,12,MUTED);

    Card((Rectangle){308,524,464,232});
    Text("TEMPERATURE",332,547,12,MUTED);
    Text(TextFormat("%.1f",sensors->temperature_c),332,575,53,INK);
    Text("C",463,597,24,MUTED);
    Color thermal=alert ? AMBER : GOOD;
    Text(alert ? "HEAT ALERT" : "BELOW THRESHOLD",572,589,13,thermal);
    Text("Demo threshold: 30 C",572,613,12,MUTED);
    DrawRectangleRounded((Rectangle){332,648,416,7},1,8,BORDER);
    float level=fminf(fmaxf((sensors->temperature_c-20)/20,0),1);
    if (level>0) DrawRectangleRounded((Rectangle){332,648,416*level,7},1,8,thermal);
    DrawLine(540,643,540,660,AMBER);
    Text("20 C baseline",332,668,11,MUTED);
    Text("40 C heat target",667,668,11,MUTED);
    if (Button((Rectangle){332,702,170,36},sensors->heating ? "Stop heat scenario" : "Trigger heat",true,false)) {
        if (sensors->heating) SensorsStopHeat(sensors);
        else SensorsTriggerHeat(sensors);
    }
    Text(sensors->heating ? "Heating at 2 C / second" : "Baseline / cooling",526,714,12,MUTED);

    Card((Rectangle){792,524,460,232});
    Text("HANDLING IMPACTS",816,547,12,MUTED);
    Text(TextFormat("%u",sensors->impact_count),816,575,sensors->impact_count>9999 ? 34 : 53,INK);
    Text(sensors->impact_count ? "IMPACT RECORDED" : "NO IMPACTS",1000,589,13,sensors->impact_count ? AMBER : GOOD);
    Text("Cumulative since last reset",1000,613,12,MUTED);
    DrawLine(816,648,1228,648,BORDER);
    Text("A trigger records one simulated impact.",816,666,13,MUTED);
    if (Button((Rectangle){816,702,170,36},"Trigger impact",true,false)) SensorsTriggerImpact(sensors);
    Text("Reset all clears the count",1000,714,12,MUTED);

    Text("Sensors continue monitoring while delivery is paused.",308,783,13,MUTED);
    Text("DEMO DATA / NO PHYSICAL DEVICE",1022,783,11,MUTED);
}

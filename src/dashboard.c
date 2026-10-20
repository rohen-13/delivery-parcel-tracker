#include "dashboard.h"
#include "account.h"
#include "temperature_history.h"
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>

static const Color BG = {12,17,23,255}, SURFACE = {20,28,36,255};
static const Color INK = {235,240,246,255}, MUTED = {153,170,186,255};
static const Color BORDER = {43,57,69,255}, ACCENT = {203,239,105,255};
static const Color GOOD = {121,216,177,255}, AMBER = {255,185,112,255};
static Font font;
static bool custom_font;
static int export_status;
static char export_name[32];

static Account account;
static TemperatureHistory temperature_history;
static bool profile_page, show_password, login_error;
static char username[33] = "demo", password[65], edit_name[49];
static int login_focus = 1, profile_focus = 0, profile_status;
static float ui_scale = 1.0f;
static Vector2 ui_offset;
static void LoginPage(void);
static void ProfilePage(Tracker *tracker, Sensors *sensors);

void DashboardObserveSensors(const Sensors *sensors)
{
    TemperatureHistoryObserve(&temperature_history,sensors->seconds,
                              sensors->temperature_c,sensors->connected);
}

bool DashboardSignedIn(void) { return account.signed_in; }
void DashboardSetViewport(float scale, Vector2 offset)
{
    ui_scale = scale;
    ui_offset = offset;
}

static void ExportEvents(const Sensors *sensors)
{
    export_status = -1;
    for (unsigned int i = 1; i <= 999; ++i) {
        char path[4096];
        snprintf(export_name, sizeof(export_name), "events-%03u.csv", i);
        int length = snprintf(path, sizeof(path), "%s%s", GetApplicationDirectory(), export_name);
        if (length < 0 || (size_t)length >= sizeof(path)) return;
        /* Exclusive creation preserves reports from earlier exports/runs. */
        FILE *output = fopen(path, "wx");
        if (!output) {
            if (errno == EEXIST) continue;
            return;
        }
        bool written = SensorsWriteCSV(sensors, output);
        int closed = fclose(output);
        if (written && closed == 0) export_status = 1;
        return;
    }
}

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
static void TemperatureGraph(const Sensors *sensors)
{
    Card((Rectangle){308,776,464,208});
    Text("TEMPERATURE HISTORY",332,795,12,MUTED);
    Text(sensors->connected ? "Last 60 s" : "Disconnected / gaps",606,795,12,
         sensors->connected ? MUTED : AMBER);
    const Rectangle plot = {367,837,376,100};
    for (int i=0; i<3; ++i) {
        float y = plot.y+i*plot.height/2;
        Color color = i==1 ? AMBER : BORDER;
        DrawLineEx((Vector2){plot.x,y},(Vector2){plot.x+plot.width,y},1,color);
        Text(TextFormat("%d C",40-i*10),332,y-6,11,i==1 ? AMBER : MUTED);
    }
    Text("60 s ago",367,949,11,MUTED);
    Text("30 C demo threshold",474,949,11,AMBER);
    Text("Now",720,949,11,MUTED);
    if (!temperature_history.count) {
        Text("Waiting for sensor readings",402,876,15,MUTED);
        return;
    }
    Vector2 previous = {0};
    for (unsigned int i=0; i<temperature_history.count; ++i) {
        const TemperatureSample *sample = &temperature_history.samples[i];
        float age = (float)(sensors->seconds-sample->seconds);
        Vector2 point = {plot.x+plot.width*(1-age/60.0f),
                         plot.y+plot.height*(1-fminf(fmaxf((sample->temperature_c-20)/20,0),1))};
        if (i && TemperatureHistoryCanJoin(&temperature_history.samples[i-1],sample))
            DrawLineEx(previous,point,2,ACCENT);
        if (sample->fresh) DrawCircleV(point,2,sample->temperature_c>=30 ? AMBER : ACCENT);
        previous=point;
    }
}

void DashboardDraw(Tracker *tracker, Sensors *sensors)
{
    if (!account.signed_in) { LoginPage(); return; }
    if (profile_page) { ProfilePage(tracker,sensors); return; }
    float progress = TrackerProgress(tracker);
    bool alert = SensorsHeatAlert(sensors);
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    ClearBackground(BG);

    /* Shipment rail: actual state and controls, with no decorative navigation. */
    DrawRectangle(0,0,280,1040,SURFACE);
    DrawLine(280,0,280,1040,BORDER);
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
    const char *percent = TextFormat("%.0f",progress*100);
    Text(percent,28,342,56,INK);
    Text("%",40 + MeasureTextEx(font,percent,56,0).x,369,21,MUTED);
    DrawRectangleRounded((Rectangle){28,416,224,6},1,8,BORDER);
    if (progress>0) DrawRectangleRounded((Rectangle){28,416,224*progress,6},1,8,ACCENT);
    Text("20 second simulated journey",28,440,13,MUTED);
    DrawLine(28,484,252,484,BORDER);
    Text("DELIVERY CONTROLS",28,511,11,MUTED);
    const char *delivery_action = progress >= 1 ? "Delivered" : tracker->running ? "Delivery running" :
                                  tracker->elapsed_seconds > 0 ? "Resume delivery" : "Start delivery";
    if (Button((Rectangle){28,542,224,44},delivery_action,
               !tracker->running && progress<1,true)) TrackerStart(tracker);
    if (Button((Rectangle){28,598,104,44},"Pause",tracker->running,false)) TrackerPause(tracker);
    if (Button((Rectangle){144,598,108,44},"Reset all",true,false)) {
        TrackerReset(tracker);
        SensorsReset(sensors);
        TemperatureHistoryReset(&temperature_history);
    }
    if (Button((Rectangle){28,660,224,40},sensors->connected ? "Disconnect tracker" : "Reconnect tracker",true,false))
        SensorsSetConnected(sensors,!sensors->connected);
    Text(sensors->connected ? "Telemetry connected" : "Telemetry disconnected",28,714,13,
         sensors->connected ? GOOD : AMBER);
    if (export_status) {
        Text(export_status > 0 ? TextFormat("Saved %s",export_name) : "CSV export failed",28,776,13,
             export_status > 0 ? GOOD : AMBER);
        Text(export_status > 0 ? "Beside the app executable" : "Check folder access; retry export",28,800,12,MUTED);
    }
    Text("ALERT SUMMARY",28,844,11,MUTED);
    Text(!sensors->connected ? "Temperature reading is stale" : alert ? "Heat threshold exceeded" : "Temperature below threshold",28,866,13,!sensors->connected || alert ? AMBER : GOOD);
    Text(sensors->impact_count ? "Impact history recorded" : "No recorded impacts",28,887,13,sensors->impact_count ? AMBER : MUTED);
    Text("C + RAYLIB",28,960,12,MUTED);
    Text("Advanced Programming",28,983,14,INK);
    Text("Prototype / simulation",28,1007,12,MUTED);

    Text("Delivery overview",308,28,30,INK);
    Text("Follow the shipment. Inspect its handling conditions.",309,68,15,MUTED);
    DrawRectangleRounded((Rectangle){1057,33,195,32},0.4f,8,(Color){31,45,34,255});
    DrawCircle(1074,49,4,ACCENT);
    Text("SIMULATION ACTIVE",1088,41,12,ACCENT);

    if (Button((Rectangle){1057,73,195,32},"My profile",true,false)) {
        profile_page=true;
        strcpy(edit_name,account.display_name);
        profile_status=0;
    }

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
        /* Use the same cumulative distance as parcel placement, not equal thirds. */
        bool reached=progress >= 1 || progress*total>=checkpoint_distance;
        DrawCircleV(route[i],9,SURFACE);
        DrawCircleLinesV(route[i],9,reached ? ACCENT : MUTED);
        DrawCircleV(route[i],3,reached ? ACCENT : MUTED);
        float width=MeasureTextEx(font,names[i],11,0).x;
        DrawRectangleRounded((Rectangle){route[i].x-width/2-8,route[i].y+20,width+16,25},0.2f,8,BG);
        Text(names[i],route[i].x-width/2,route[i].y+27,11,INK);
        const char *checkpoint_state = reached ? "Reached" : "Upcoming";
        float state_width = MeasureTextEx(font,checkpoint_state,12,0).x;
        Text(checkpoint_state,route[i].x-state_width/2,route[i].y+49,12,reached ? ACCENT : MUTED);
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
    Text("Reached checkpoints use route distance",987,472,12,MUTED);

    Card((Rectangle){308,524,464,232});
    Text("TEMPERATURE",332,547,12,MUTED);
    Text(TextFormat("%.1f",sensors->temperature_c),332,575,53,INK);
    Text("C",463,597,24,MUTED);
    Color thermal=alert || !sensors->connected ? AMBER : GOOD;
    Text(!sensors->connected ? "STALE READING" : alert ? "HEAT ALERT" : "BELOW THRESHOLD",572,589,13,thermal);
    Text(!sensors->connected ? TextFormat("Last reading %.1fs ago",sensors->stale_seconds) : "Demo threshold: 30 C",572,613,12,MUTED);
    DrawRectangleRounded((Rectangle){332,648,416,7},1,8,BORDER);
    float level=fminf(fmaxf((sensors->temperature_c-20)/20,0),1);
    if (level>0) DrawRectangleRounded((Rectangle){332,648,416*level,7},1,8,thermal);
    DrawLine(540,643,540,660,AMBER);
    Text("20 C baseline",332,668,11,MUTED);
    Text("40 C heat target",667,668,11,MUTED);
    if (Button((Rectangle){332,702,170,36},sensors->heating ? "Stop heat scenario" : "Trigger heat",sensors->connected,false)) {
        if (sensors->heating) SensorsStopHeat(sensors);
        else SensorsTriggerHeat(sensors);
    }
    Text(!sensors->connected ? "Monitoring suspended" : sensors->heating ? "Heating at 2 C / second" : "Baseline / cooling",526,714,12,MUTED);

    Card((Rectangle){792,524,460,232});
    Text("HANDLING IMPACTS",816,547,12,MUTED);
    Text(TextFormat("%u",sensors->impact_count),816,575,sensors->impact_count>9999 ? 34 : 53,INK);
    Text(!sensors->connected ? "LAST KNOWN COUNT" : sensors->impact_count ? "IMPACT RECORDED" : "NO IMPACTS",1000,589,13,sensors->impact_count || !sensors->connected ? AMBER : GOOD);
    Text("Cumulative since last reset",1000,613,12,MUTED);
    DrawLine(816,648,1228,648,BORDER);
    Text("A trigger records one simulated impact.",816,666,13,MUTED);
    if (Button((Rectangle){816,702,170,36},"Trigger impact",sensors->connected,false)) SensorsTriggerImpact(sensors);
    Text("Reset all clears the count",1000,714,12,MUTED);

    TemperatureGraph(sensors);
    Card((Rectangle){792,776,460,208});
    Text("RECENT EVENTS",816,796,12,MUTED);
    if (Button((Rectangle){1050,784,178,36},"Export CSV (E)",true,false) || IsKeyPressed(KEY_E))
        ExportEvents(sensors);
    Text("Latest 3 / 32 retained for export",816,825,11,MUTED);
    unsigned int first = sensors->event_count > 3 ? sensors->event_count - 3 : 0;
    for (unsigned int i=first; i<sensors->event_count; ++i) {
        const SensorEvent *event = &sensors->events[i];
        float y = 854 + (i-first)*37;
        Text(TextFormat("%.1f s",event->seconds),816,y,12,MUTED);
        Text(SensorsEventName(event->kind),900,y,14,INK);
    }
    Text("Sensors monitor while paused; disconnection freezes readings.",308,1005,13,MUTED);
    Text("DEMO DATA / NO PHYSICAL DEVICE",1022,1005,11,MUTED);
}

static void Input(Rectangle bounds, char *buffer, size_t capacity, int id,
                  int *focus, bool secret, const char *hint)
{
    bool hover = CheckCollisionPointRec(GetMousePosition(), bounds);
    if (hover) SetMouseCursor(MOUSE_CURSOR_IBEAM);
    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) *focus = id;
    bool active = *focus == id;
    if (active) {
        int key;
        while ((key = GetCharPressed()) != 0) {
            size_t n = strlen(buffer);
            if (key >= 32 && key <= 126 && n + 1 < capacity) {
                buffer[n] = (char)key;
                buffer[n+1] = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            size_t n = strlen(buffer);
            if (n) buffer[n-1] = '\0';
        }
    }
    DrawRectangleRounded(bounds, 0.12f, 8, BG);
    DrawRectangleRoundedLinesEx(bounds, 0.12f, 8, 1, active ? ACCENT : BORDER);
    char masked[65];
    size_t n = strlen(buffer);
    if (n >= sizeof(masked)) n = sizeof(masked)-1;
    memset(masked, '*', n);
    masked[n] = '\0';
    const char *value = buffer[0] ? (secret ? masked : buffer) : hint;
    /* Clip long input so it cannot overlap the form or adjacent controls. */
    int scissor_x = (int)(bounds.x * ui_scale + ui_offset.x);
    int scissor_y = (int)(bounds.y * ui_scale + ui_offset.y);
    BeginScissorMode(scissor_x, scissor_y, (int)(bounds.width * ui_scale), (int)(bounds.height * ui_scale));
    float width = buffer[0] ? MeasureTextEx(font, value, 20, 0).x : 0;
    float shift = buffer[0] && width > bounds.width-36 ? width-(bounds.width-36) : 0;
    Text(value, bounds.x+16-shift, bounds.y+17, 20, buffer[0] ? INK : MUTED);
    if (active && ((int)(GetTime()*2) % 2 == 0))
        DrawRectangle((int)(bounds.x+18+width-shift), (int)bounds.y+15, 2, 25, ACCENT);
    EndScissorMode();
}

static void LoginPage(void)
{
    ClearBackground(BG);
    Text("PARCEL / OPS",80,65,24,ACCENT);
    Text("DELIVERY MONITORING",80,180,13,MUTED);
    Text("Every parcel.",80,220,52,INK);
    Text("Every checkpoint.",80,282,52,INK);
    Text("Monitor a shipment and its handling conditions",80,377,19,MUTED);
    Text("from one place.",80,407,19,MUTED);
    DrawLineEx((Vector2){95,575}, (Vector2){470,575}, 4, BORDER);
    const char *stops[] = {"DEPOT", "HUB", "DELIVERED"};
    for (int i=0; i<3; ++i) {
        DrawCircle(95+i*187,575,9,i==0 ? ACCENT : MUTED);
        Text(stops[i],78+i*187,604,12,MUTED);
    }
    Text("C + RAYLIB  /  SIMULATED IOT",80,920,13,MUTED);
    Card((Rectangle){620,160,580,720});
    Text("Welcome back",660,202,34,INK);
    Text("Sign in to the parcel dashboard",660,253,18,MUTED);
    Text("USERNAME",660,312,12,MUTED);
    if (IsKeyPressed(KEY_TAB)) login_focus = login_focus == 0 ? 1 : 0;
    Input((Rectangle){660,338,500,60}, username, sizeof(username), 0, &login_focus, false, "Username");
    Text("PASSWORD",660,426,12,MUTED);
    Input((Rectangle){660,452,500,60}, password, sizeof(password), 1, &login_focus, !show_password, "Password");
    if (Button((Rectangle){990,526,170,36},show_password ? "Hide password" : "Show password",true,false)) show_password=!show_password;
    if (login_error) Text("Use the demo username and password below.",660,583,17,AMBER);
    if (Button((Rectangle){660,622,500,48},"Sign in",true,true) || IsKeyPressed(KEY_ENTER)) {
        if (AccountSignIn(&account,username,password)) {
            memset(password,0,sizeof(password));
            show_password=false;
            login_error=false;
            profile_page=false;
        } else login_error=true;
    }
    Text("DEMO ACCOUNT",660,714,12,ACCENT);
    Text("Username: demo   /   Password: parcel123",660,742,18,INK);
    Text("Local prototype login. No online account or secure",660,790,16,MUTED);
    Text("authentication service is connected.",660,815,16,MUTED);
}

static void ProfilePage(Tracker *tracker, Sensors *sensors)
{
    ClearBackground(BG);
    Text("PARCEL / OPS",80,65,24,ACCENT);
    if (Button((Rectangle){920,55,280,44},"Back to dashboard",true,false)) profile_page=false;
    Text("My profile",160,170,42,INK);
    Text("Your operator details for this session",160,225,19,MUTED);
    Card((Rectangle){160,290,960,560});
    DrawCircle(230,367,32,ACCENT);
    Text("P",218,345,36,BG);
    float name_width = MeasureTextEx(font,account.display_name,28,0).x;
    float name_size = name_width > 790 ? 28*790/name_width : 28;
    Text(account.display_name,290,333,name_size,INK);
    Text("Parcel operator  /  Demo account",290,377,16,MUTED);
    DrawLine(200,435,1080,435,BORDER);
    Text("DISPLAY NAME",200,470,12,MUTED);
    Input((Rectangle){200,500,560,60},edit_name,sizeof(edit_name),0,&profile_focus,false,"Your display name");
    if (Button((Rectangle){790,506,290,48},"Save changes",true,true) || IsKeyPressed(KEY_ENTER))
        profile_status=AccountSetDisplayName(&account,edit_name) ? 1 : -1;
    if (profile_status) Text(profile_status>0 ? "Display name saved for this session." : "Enter a name using 1 to 48 printable characters.",200,585,17,profile_status>0 ? GOOD : AMBER);
    Text("ACCOUNT",200,646,12,MUTED);
    Text("demo",200,675,21,INK);
    Text("ASSIGNED SHIPMENT",650,646,12,MUTED);
    Text("PKG-001",650,675,21,INK);
    Text("Profile edits last until sign-out or closing the app.",200,742,17,MUTED);
    Text("Signing out clears the current delivery and sensor session.",200,775,17,MUTED);
    if (Button((Rectangle){160,888,240,48},"Sign out",true,false)) {
        AccountSignOut(&account);
        TrackerReset(tracker);
        SensorsReset(sensors);
        TemperatureHistoryReset(&temperature_history);
        memset(edit_name,0,sizeof(edit_name));
        memset(password,0,sizeof(password));
        profile_page=false;
        login_focus=0;
        profile_status=0;
        export_status=0;
        show_password=false;
        login_error=false;
    }
    Text("DEMO ACCOUNT  /  NO PASSWORDS SAVED",650,908,13,MUTED);
}

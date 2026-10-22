#include "dashboard.h"
#include "account.h"
#include "temperature_history.h"
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

static const Color BG = {12,17,23,255}, SURFACE = {20,28,36,255};
static const Color INK = {235,240,246,255}, MUTED = {153,170,186,255};
static const Color BORDER = {43,57,69,255}, ACCENT = {203,239,105,255};
static const Color GOOD = {121,216,177,255}, AMBER = {255,185,112,255};
static Font font;
static bool custom_font;
static int export_status[5];
static char export_name[5][64];

static Account account;
static bool profile_page, show_password, login_error;
static char username[33] = "demo", password[65], edit_name[49];
static int login_step, login_focus, profile_focus = 0, profile_status;
static float ui_scale = 1.0f;
static Vector2 ui_offset;
static void LoginPage(DeliveryRound *round);
static void ProfilePage(DeliveryRound *round);

bool DashboardSignedIn(void) { return account.signed_in; }
bool DashboardAccountPage(void) { return !account.signed_in || account.locked || profile_page; }
void DashboardLock(void)
{
    if(!AccountLock(&account))return;
    memset(password,0,sizeof(password));
    strcpy(username,"demo");
    login_step=1;login_focus=1;show_password=false;login_error=false;
}
void DashboardOpenProfile(void)
{
    if (!account.signed_in || account.locked) return;
    profile_page = true;
    strcpy(edit_name, account.display_name);
    profile_status = 0;
}
void DashboardSetViewport(float scale, Vector2 offset)
{
    ui_scale = scale;
    ui_offset = offset;
}

static void ExportEvents(const DeliveryRound *round)
{
    int i=round->selected;
    export_status[i]=RoundExportCSV(round,GetApplicationDirectory(),export_name[i],sizeof(export_name[i]))?1:-1;
}
void DashboardClearExportStatus(void)
{
    memset(export_status,0,sizeof(export_status));
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
static void TemperatureGraph(const Sensors *sensors, const TemperatureHistory *history,bool archived)
{
    Card((Rectangle){316,492,548,492});
    Text("Temperature history",340,516,20,INK);
    Text(archived?"60 seconds before delivery":sensors->connected?"Last 60 seconds":"Disconnected / stale readings",340,552,13,MUTED);
    Rectangle plot={376,616,460,268};
    for(int i=0;i<3;++i) {
        float y=plot.y+i*plot.height/2;
        DrawLineEx((Vector2){plot.x,y},(Vector2){plot.x+plot.width,y},1,i==1?AMBER:BORDER);
        Text(TextFormat("%d C",40-i*10),334,y-6,12,MUTED);
    }
    for(unsigned i=0;i<history->count;++i) {
        const TemperatureSample *sample=&history->samples[i];
        float x=plot.x+plot.width*(1-(float)(sensors->seconds-sample->seconds)/60);
        float y=plot.y+plot.height*(1-fminf(fmaxf((sample->temperature_c-20)/20,0),1));
        if(i && TemperatureHistoryCanJoin(&history->samples[i-1],sample)) {
            const TemperatureSample *previous=&history->samples[i-1];
            Vector2 a={plot.x+plot.width*(1-(float)(sensors->seconds-previous->seconds)/60),
                       plot.y+plot.height*(1-fminf(fmaxf((previous->temperature_c-20)/20,0),1))};
            DrawLineEx(a,(Vector2){x,y},2,ACCENT);
        }
        if(sample->fresh)DrawCircleV((Vector2){x,y},2,sample->temperature_c>=30?AMBER:ACCENT);
    }
    if(!history->count)Text("Waiting for readings",490,740,16,MUTED);
    Text("60 s ago",376,922,12,MUTED);
    Text("30 C demo threshold",520,922,12,AMBER);
    Text("Now",808,922,12,MUTED);
}

void DashboardDraw(DeliveryRound *round)
{
    if(!account.signed_in || account.locked) {LoginPage(round);return;}
    if(profile_page)ProfilePage(round);
}

int DashboardDrawMonitoring(DeliveryRound *round)
{
    ClearBackground(BG);
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    Text("Parcel Ops",28,32,26,INK);
    Text("Package monitoring",316,28,32,INK);
    if(Button((Rectangle){800,28,204,40},"View on dispatch map",true,false))return 1;
    if(Button((Rectangle){1020,28,104,40},"Profile",true,false))return 2;
    if(Button((Rectangle){1136,28,116,40},"Lock",true,false))return 3;
    Text("Packages",28,132,20,INK);
    Text("Choose a parcel to monitor",28,166,15,MUTED);
    if(IsKeyPressed(KEY_DOWN))RoundSelect(round,(round->selected+1)%5);
    if(IsKeyPressed(KEY_UP))RoundSelect(round,(round->selected+4)%5);
    for(int i=0;i<5;++i) {
        float y=218+i*100;
        if(Button((Rectangle){24,y,240,64},round->packages[i].id,true,i==round->selected))RoundSelect(round,i);
        Text(round->packages[i].destination,32,y+72,13,MUTED);
    }
    Text("Simulated sensor readings",28,940,14,MUTED);
    int issues=RoundIssueCount(round);
    if(Button((Rectangle){24,784,240,44},"Next parcel to check (N)",issues>0,false) || (issues>0&&IsKeyPressed(KEY_N)))
        RoundSelectNextIssue(round);
    Text(TextFormat("%d undelivered parcels flagged",issues),28,852,14,issues?AMBER:GOOD);
    Text("Heat / impact history / offline",28,882,13,MUTED);
    Text("Up / Down  Select package",28,972,13,MUTED);
    DrawLine(288,0,288,1040,BORDER);
    int index=round->selected;
    Package *package=&round->packages[index];
    Sensors *sensors=&round->sensors[index];
    Text(TextFormat("%s / %s",package->id,package->destination),316,82,20,INK);
    const char *delivery=package->delivered?"Delivered / readings archived":"Not included in the planned round";
    if(!package->delivered && package->included)delivery="Included / route not planned";
    if(!package->delivered && round->route.ready)
        for(int i=0;i<round->route.count;++i)if(round->route.order[i]==index)
            delivery=TextFormat("Planned delivery stop %d of %d",i+1,round->route.count);
    Text(delivery,316,122,16,package->delivered?GOOD:MUTED);
    bool active=!package->delivered;
    bool alert=SensorsHeatAlert(sensors);
    Card((Rectangle){316,184,448,284});
    Text(package->id,340,208,16,MUTED);
    Text(TextFormat("%.1f C",sensors->temperature_c),340,248,38,INK);
    Text(package->delivered?"Archived at delivery":!sensors->connected?"Stale sensor reading":alert?"Heat threshold exceeded":"Temperature within demo range",
         340,306,13,!sensors->connected||alert?AMBER:GOOD);
    if(Button((Rectangle){340,350,400,44},sensors->heating?"Stop heat scenario":"Trigger heat",active&&sensors->connected,false)) {
        if(sensors->heating)SensorsStopHeat(sensors);else SensorsTriggerHeat(sensors);
    }
    Text(!sensors->connected?TextFormat("Last reading %.1f s ago",sensors->stale_seconds):TextFormat("Peak %.1f C / Exposure %.1f s",sensors->peak_temperature_c,sensors->heat_seconds),340,426,12,MUTED);
    Card((Rectangle){788,184,464,284});
    Text("Handling and connection",812,208,18,INK);
    Text(TextFormat("%u impacts recorded",sensors->impact_count),812,248,22,sensors->impact_count?AMBER:INK);
    if(Button((Rectangle){812,300,416,40},"Record impact",active&&sensors->connected,false))SensorsTriggerImpact(sensors);
    if(Button((Rectangle){812,356,416,40},sensors->connected?"Disconnect sensor":"Reconnect sensor",active,false))
        SensorsSetConnected(sensors,!sensors->connected);
    if(Button((Rectangle){812,412,416,40},"Export package events (E)",true,false)||IsKeyPressed(KEY_E))ExportEvents(round);
    TemperatureGraph(sensors,&round->histories[index],package->delivered);
    Card((Rectangle){888,492,364,492});
    Text("Recent package events",912,516,20,INK);
    Text(package->delivered?"History frozen at delivery":"Latest 7 / 32 retained",912,552,13,MUTED);
    unsigned first=sensors->event_count>7?sensors->event_count-7:0;
    for(unsigned i=first;i<sensors->event_count;++i) {
        const SensorEvent *event=&sensors->events[i];
        float y=600+(i-first)*40;
        Text(TextFormat("%.1fs",event->seconds),912,y,12,MUTED);
        Text(SensorsEventName(event->kind),976,y,13,INK);
    }
    Text(TextFormat("Unobserved time %.1f s",sensors->unobserved_seconds),912,938,12,MUTED);
    if(export_status[index])Text(export_status[index]>0?"Package CSV saved beside app":"Export failed. Check folder access.",912,958,12,export_status[index]>0?GOOD:AMBER);
    Text(TextFormat("%s / %s / Simulated telemetry",package->id,package->destination),316,1008,13,MUTED);
    return 0;
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

static void SignOut(void)
{
    AccountSignOut(&account);
    memset(edit_name,0,sizeof(edit_name));
    memset(password,0,sizeof(password));
    strcpy(username,"demo");
    profile_page=false;login_focus=0;login_step=0;
    profile_status=0;memset(export_status,0,sizeof(export_status));show_password=false;login_error=false;
}

static void LoginPage(DeliveryRound *round)
{
    (void)round;
    ClearBackground(BG);
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    DrawRectangle(0,0,568,1040,SURFACE);
    DrawLine(568,0,568,1040,BORDER);
    DrawRectangleRounded((Rectangle){64,64,36,36},0.2f,8,ACCENT);
    DrawRectangleLinesEx((Rectangle){73,73,18,19},2,BG);
    Text("Parcel Ops",116,67,26,INK);
    Text(account.locked?"Your round is":"Ready for your",64,192,44,INK);
    Text(account.locked?"right here.":"next delivery?",64,246,44,INK);
    Text("Plan the stops. Follow the route.",64,326,20,MUTED);
    Text("Keep an eye on every parcel.",64,358,20,MUTED);
    DrawLineEx((Vector2){96,496},(Vector2){96,712},2,BORDER);
    const char *labels[]={"Plan the delivery round","Compare driving routes","Monitor parcel conditions"};
    const char *details[]={"Choose the packages for this trip","Compare road distances between stops","See heat, impacts and connection gaps"};
    for(int i=0;i<3;++i) {
        int y=480+i*104;
        DrawCircle(96,y+12,12,i==0?ACCENT:BORDER);
        DrawCircle(96,y+12,4,i==0?BG:MUTED);
        Text(labels[i],128,y,19,INK);
        Text(details[i],128,y+33,15,MUTED);
    }
    Text("C + Raylib  /  Local demonstration",64,944,15,MUTED);
    if(account.locked) {
        Text("Session retained",680,138,16,GOOD);
    } else {
        Text("Account",680,138,16,login_step==0?ACCENT:MUTED);
        DrawLine(766,147,818,147,BORDER);
        Text("Password",842,138,16,login_step==1?ACCENT:MUTED);
    }
    Text(account.locked?"Workspace locked":login_step?"Welcome back":"Sign in to Parcel Ops",680,214,36,INK);
    Text(account.locked?"Unlock to continue your delivery round.":login_step?"One workspace for dispatch and monitoring.":"Start with your operator account.",680,272,18,MUTED);
    if(login_step==0 && !account.locked) {
        Text("Operator username",680,348,16,INK);
        Input((Rectangle){680,378,480,64},username,sizeof(username),0,&login_focus,false,"Enter your username");
        Text("For this demonstration, use demo.",680,464,16,MUTED);
        bool advance=Button((Rectangle){680,520,480,52},"Continue to password",username[0]!=0,true);
        if(username[0] && (advance || IsKeyPressed(KEY_ENTER))) {
            login_step=1;login_focus=1;login_error=false;
        }
    } else {
        Text(account.locked?account.display_name:username,680,334,22,INK);
        if(!account.locked && Button((Rectangle){1024,324,136,40},"Change",true,false)) {
            login_step=0;login_focus=0;login_error=false;
            memset(password,0,sizeof(password));show_password=false;
        }
        Text("Password",680,404,16,INK);
        Input((Rectangle){680,436,480,64},password,sizeof(password),1,&login_focus,!show_password,"Enter your password");
        if(Button((Rectangle){990,516,170,36},show_password?"Hide password":"Show password",true,false))show_password=!show_password;
        if(login_error)Text("Details did not match. Check them and try again.",680,578,16,AMBER);
        bool submit=Button((Rectangle){680,624,480,52},account.locked?"Unlock workspace":"Open workspace",password[0]!=0,true);
        if(password[0] && (submit || IsKeyPressed(KEY_ENTER))) {
            bool was_locked=account.locked;
            bool accepted=was_locked?AccountUnlock(&account,username,password):AccountSignIn(&account,username,password);
            if(accepted) {
                if(!was_locked)profile_page=false;
                login_error=false;
            } else login_error=true;
            memset(password,0,sizeof(password));
            show_password=false;
        }
        if(account.locked) {
            Text("Monitoring continues. Your packages and route are retained.",680,708,15,MUTED);
            if(Button((Rectangle){680,738,200,32},"Sign out instead",true,false))SignOut();
        }
    }
    DrawLine(680,780,1160,780,BORDER);
    Text("Demo access",680,806,18,INK);
    Text("Username: demo     Password: parcel123",680,843,16,MUTED);
    Text("Local prototype only. Do not enter a personal password.",680,889,15,MUTED);
    Text("No online account service is connected.",680,917,15,MUTED);
}

static void ProfilePage(DeliveryRound *round)
{
    (void)round;
    ClearBackground(BG);
    Text("PARCEL / OPS",80,65,24,ACCENT);
    if (Button((Rectangle){920,55,280,44},"Back to workspace",true,false)) profile_page=false;
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
    Text("WORKSPACE",650,646,12,MUTED);
    Text("Dispatch + sensor demo",650,675,21,INK);
    Text("Profile edits last until sign-out or closing the app.",200,742,17,MUTED);
    Text("Signing out clears packages, routes and the sensor session.",200,775,17,MUTED);
    if (Button((Rectangle){160,888,240,48},"Sign out",true,false)) {
        SignOut();
    }
    if(Button((Rectangle){424,888,240,48},"Lock workspace",true,false))DashboardLock();
    Text("DEMO ACCOUNT  /  NO PASSWORDS SAVED",650,908,13,MUTED);
}

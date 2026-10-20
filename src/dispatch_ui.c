#define _POSIX_C_SOURCE 200809L
#include "dispatch_ui.h"
#include "dispatch.h"
#include "raylib.h"
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static const Color BG={12,17,23,255}, PANEL={20,28,36,255}, INK={235,240,246,255};
static const Color MUTED={153,170,186,255}, ACCENT={203,239,105,255}, AMBER={255,185,112,255};
static Package packages[5], request_packages[5];
static DeliveryRoute route, response;
static Font font;
static bool logged, custom_font, busy;
static char user[40]="", password[40]="", login_error[80]="";
static int focus=0, selected=0, job, tile_x,tile_y;
static pthread_t worker;
static atomic_bool complete, cancel;
static atomic_int tiles_processed;
static Texture2D tiles[12];
static char cache[4096], network_message[160]="Map not loaded. Load the visible area when online.";
static double last_request=-100;
static const Rectangle MAP={360,178,892,669};
static double X(double lon) {return (lon+180)/360*16384;}
static double Y(double lat) {double r=lat*3.141592653589793/180;return (1-asinh(tan(r))/3.141592653589793)/2*16384;}
static Vector2 Project(GeoPoint point)
{
    return (Vector2){MAP.x+(float)((X(point.lon)-tile_x)*256)*MAP.width/1024,
                     MAP.y+(float)((Y(point.lat)-tile_y)*256)*MAP.height/768};
}
static void Text(const char *s,float x,float y,float size,Color color)
{DrawTextEx(font,s,(Vector2){x,y},size,0,color);}
static bool Button(Rectangle rect,const char *label,bool enabled)
{
    bool hover=enabled && CheckCollisionPointRec(GetMousePosition(),rect);
    DrawRectangleRounded(rect,0.12f,8,hover?(Color){49,66,46,255}:PANEL);
    DrawRectangleRoundedLinesEx(rect,0.12f,8,1,enabled?ACCENT:MUTED);
    Text(label,rect.x+14,rect.y+13,15,enabled?INK:MUTED);
    if(hover)SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static void TilePath(char *path,size_t size,int i)
{snprintf(path,size,"%s/14-%d-%d.png",cache,tile_x+i%4,tile_y+i/4);}
static void *Work(void *unused)
{
    (void)unused;
    if(job==1) {
        unsigned char *data=malloc(1048577);int saved=0;
        for(int i=0;data && i<12 && !atomic_load(&cancel);++i) {
            char path[4096],url[160];TilePath(path,sizeof(path),i);struct stat info;
            if(stat(path,&info)==0 && time(NULL)-info.st_mtime<7*24*3600) ++saved;
            else {
                snprintf(url,sizeof(url),"https://tile.openstreetmap.org/14/%d/%d.png",tile_x+i%4,tile_y+i/4);
                size_t size;
                if(DispatchFetch(url,NULL,data,1048577,&size) && size>8 && memcmp(data,"\x89PNG\r\n\x1a\n",8)==0) {
                    FILE *out=fopen(path,"wb");
                    if(out) {bool ok=fwrite(data,1,size,out)==size;int closed=fclose(out);if(ok&&closed==0)++saved;}
                }
            }
            atomic_store(&tiles_processed,i+1);
        }
        free(data);
        snprintf(response.message,sizeof(response.message),"%d of 12 visible OSM tiles available. Missing tiles can be retried.",saved);
    } else DispatchPlan(request_packages,&response);
    atomic_store(&complete,true);return NULL;
}
static void Start(int kind)
{
    if(busy || GetTime()-last_request<5)return;
    last_request=GetTime();job=kind;response=(DeliveryRoute){0};
    memcpy(request_packages,packages,sizeof(packages));
    atomic_store(&complete,false);atomic_store(&tiles_processed,0);
    if(pthread_create(&worker,NULL,Work,NULL)==0)busy=true;
    else snprintf(network_message,sizeof(network_message),"Could not start network request. Please retry.");
}
void DispatchUIInit(void)
{
    DispatchSeed(packages);tile_x=(int)floor(X(-7.113))-2;tile_y=(int)floor(Y(52.256))-1;
    snprintf(cache,sizeof(cache),"%smap-cache",GetApplicationDirectory());
    if(mkdir(cache,0700)!=0 && !DirectoryExists(cache))
        snprintf(network_message,sizeof(network_message),"Map cache is not writable. Routing is still available.");
    const char *path="/System/Library/Fonts/Supplemental/Arial.ttf";
    custom_font=FileExists(path);font=custom_font?LoadFontEx(path,64,NULL,0):GetFontDefault();
    SetTextureFilter(font.texture,TEXTURE_FILTER_BILINEAR);
}
static void Edit(char *buffer,size_t capacity)
{
    int key;
    while((key=GetCharPressed())>0) if(key>=32 && key<127 && strlen(buffer)+1<capacity) {
        size_t n=strlen(buffer);buffer[n]=(char)key;buffer[n+1]=0;
    }
    if(IsKeyPressed(KEY_BACKSPACE) && buffer[0])buffer[strlen(buffer)-1]=0;
}
static void Login(void)
{
    Text("PARCEL / OPS",104,124,26,ACCENT);
    Text("Your delivery round",104,216,52,INK);
    Text("Plan several stops on real roads.",104,294,22,MUTED);
    Text("Local demonstration only",104,366,20,AMBER);
    Text("No real accounts, encrypted storage or access protection.",104,404,17,MUTED);
    Text("Do not enter a personal password.",104,434,17,MUTED);
    Text("Demo username: demo",104,510,19,INK);
    Text("Demo password: parcel-demo",104,544,19,INK);
    Text("Sign in to the demo",760,246,30,INK);
    Rectangle fields[2]={{760,332,400,52},{760,442,400,52}};
    Text("Username",760,302,16,MUTED);Text("Password",760,412,16,MUTED);
    for(int i=0;i<2;++i) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),fields[i]))focus=i;
        DrawRectangleRounded(fields[i],0.1f,8,PANEL);
        DrawRectangleRoundedLinesEx(fields[i],0.1f,8,1,focus==i?ACCENT:MUTED);
    }
    if(IsKeyPressed(KEY_TAB))focus=1-focus;
    Edit(focus==0?user:password,40);
    char masked[40];memset(masked,'*',strlen(password));masked[strlen(password)]=0;
    Text(user,774,348,18,INK);Text(masked,774,458,18,INK);
    if(Button((Rectangle){760,534,400,48},"Sign in",true) || IsKeyPressed(KEY_ENTER)) {
        if(DispatchLogin(user,password)) {logged=true;memset(password,0,sizeof(password));login_error[0]=0;}
        else {snprintf(login_error,sizeof(login_error),"Use the demo credentials shown on the left.");memset(password,0,sizeof(password));}
    }
    Text(login_error,760,602,15,AMBER);
    Text("Waterford demo stops. No device location is collected.",104,944,16,MUTED);
}
int DispatchUIDraw(void)
{
    if(busy && atomic_load(&complete)) {
        pthread_join(worker,NULL);busy=false;
        if(job==1) {
            snprintf(network_message,sizeof(network_message),"%s",response.message);
            for(int i=0;i<12;++i) {
                if(tiles[i].id)UnloadTexture(tiles[i]);
                char path[4096];TilePath(path,sizeof(path),i);
                tiles[i]=FileExists(path)?LoadTexture(path):(Texture2D){0};
            }
        } else route=response;
    }
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);ClearBackground(BG);
    if(!logged){Login();return 0;}
    Text("PARCEL / OPS",28,30,24,ACCENT);
    Text("Delivery round",360,28,36,INK);
    Text("Waterford  /  demo packages  /  fixed depot",360,78,17,MUTED);
    if(Button((Rectangle){1060,24,192,44},"Sensor monitoring",!busy))return 1;
    Text("Packages",28,122,25,INK);
    Text("Up / Down selects. Space includes.",28,158,15,MUTED);
    if(!busy) {
        if(IsKeyPressed(KEY_DOWN))selected=(selected+1)%5;
        if(IsKeyPressed(KEY_UP))selected=(selected+4)%5;
        if(IsKeyPressed(KEY_SPACE) && !packages[selected].delivered) {
            packages[selected].included=!packages[selected].included;route=(DeliveryRoute){0};
            snprintf(route.message,sizeof(route.message),"Stops changed. Plan a new route.");
        }
    }
    for(int i=0;i<5;++i) {
        float y=204+i*106;Rectangle row={20,y,316,94};
        DrawRectangleRounded(row,0.1f,8,i==selected?(Color){38,53,43,255}:PANEL);
        Text(packages[i].id,70,y+12,18,INK);
        Text(packages[i].destination,70,y+39,15,MUTED);
        Text(packages[i].delivered?"Delivered":packages[i].included?"Included":"Excluded",70,y+65,13,packages[i].delivered?ACCENT:MUTED);
        Rectangle check={32,y+18,26,26};DrawRectangleLinesEx(check,2,packages[i].included?ACCENT:MUTED);
        if(packages[i].included)DrawRectangle(38,(int)y+24,14,14,ACCENT);
        if(!busy && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),row)) {
            selected=i;
            if(CheckCollisionPointRec(GetMousePosition(),check) && !packages[i].delivered) {
                packages[i].included=!packages[i].included;route=(DeliveryRoute){0};
                snprintf(route.message,sizeof(route.message),"Stops changed. Plan a new route.");
            }
        }
    }
    if(Button((Rectangle){28,758,300,44},"Mark selected delivered",!busy && !packages[selected].delivered)) {
        packages[selected].delivered=true;packages[selected].included=false;route=(DeliveryRoute){0};
        snprintf(route.message,sizeof(route.message),"Delivery recorded. Replan remaining stops from the depot.");
    }
    if(Button((Rectangle){28,818,300,44},"Reset demo packages",!busy)) {DispatchSeed(packages);route=(DeliveryRoute){0};}
    if(Button((Rectangle){28,946,138,44},"Sign out",!busy)) {logged=false;user[0]=0;password[0]=0;}
    bool cooldown=GetTime()-last_request>=5;
    if(Button((Rectangle){360,112,196,44},"Load OSM map (L)",!busy && cooldown) || IsKeyPressed(KEY_L))Start(1);
    if(Button((Rectangle){572,112,242,44},"Plan distance route (P)",!busy && cooldown) || IsKeyPressed(KEY_P))Start(2);
    Text("Depot to last stop. No return leg.",836,127,16,MUTED);
    BeginScissorMode((int)MAP.x,(int)MAP.y,(int)MAP.width,(int)MAP.height);
    DrawRectangleRec(MAP,PANEL);
    for(int i=0;i<12;++i) {
        Rectangle dest={MAP.x+(i%4)*MAP.width/4,MAP.y+(i/4)*MAP.height/3,MAP.width/4,MAP.height/3};
        if(tiles[i].id)DrawTexturePro(tiles[i],(Rectangle){0,0,256,256},dest,(Vector2){0},0,WHITE);
        else DrawRectangleLinesEx(dest,1,(Color){43,57,69,255});
    }
    if(route.ready)for(int i=1;i<route.point_count;++i)
        DrawLineEx(Project(route.points[i-1]),Project(route.points[i]),4,(Color){29,86,207,255});
    Vector2 depot=Project(DISPATCH_DEPOT);DrawCircleV(depot,11,(Color){20,28,36,255});
    Text("D",depot.x-6,depot.y-8,16,WHITE);
    for(int i=0;i<5;++i) {
        Vector2 pos=Project(packages[i].point);
        DrawCircleV(pos,i==selected?16:12,packages[i].delivered?DARKGREEN:packages[i].included?(Color){161,59,25,255}:GRAY);
        Text(TextFormat("%d",i+1),pos.x-5,pos.y-8,16,WHITE);
    }
    EndScissorMode();
    DrawRectangle(360,824,510,23,PANEL);
    Text("Map data (c) OpenStreetMap contributors / ODbL",370,829,13,INK);
    if(CheckCollisionPointRec(GetMousePosition(),(Rectangle){360,824,510,23}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        OpenURL("https://www.openstreetmap.org/copyright");
    if(busy)Text(job==1?TextFormat("Loading visible map tiles: %d / 12",atomic_load(&tiles_processed)):"Requesting driving distances and route...",360,872,18,ACCENT);
    else Text(route.message[0]?route.message:network_message,360,872,16,route.ready?ACCENT:MUTED);
    if(route.ready) {
        Text(TextFormat("%.2f km  /  %.0f min estimated",route.distance_km,route.duration_seconds/60),360,910,26,INK);
        char order[140]="Depot";
        for(int i=0;i<route.count;++i)strcat(order,TextFormat(" > %s",packages[route.order[i]].id));
        Text(order,360,952,17,INK);
    } else Text("Select packages, then plan. Network failure never shows a fake route.",360,920,17,MUTED);
    Text("Valhalla online routing. Exact stop order against its distance matrix; no live GPS.",360,1008,14,MUTED);
    return 0;
}
void DispatchUIClose(void)
{
    atomic_store(&cancel,true);if(busy)pthread_join(worker,NULL);
    for(int i=0;i<12;++i)if(tiles[i].id)UnloadTexture(tiles[i]);
    if(custom_font)UnloadFont(font);
    memset(password,0,sizeof(password));
}

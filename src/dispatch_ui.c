#define _POSIX_C_SOURCE 200809L
#include "dispatch_ui.h"
#include "dispatch.h"
#include "dashboard.h"
#include "round_store.h"
#include "ui_typography.h"
#include "raylib.h"
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static const Color BG={12,18,29,255}, PANEL={23,32,46,255}, INK={240,245,250,255};
static const Color MUTED={170,185,203,255}, ACCENT={92,224,196,255}, BORDER={55,72,90,255};
static DeliveryRound *workspace;
static Package request_packages[5];
static DeliveryRoute response;
static Font font,heading_font;
static bool custom_font, busy;
static int job, tile_x,tile_y;
static unsigned revision, request_revision;
static GeoPoint request_position;
static float ui_scale = 1;
static Vector2 ui_offset;
static pthread_t worker;
static atomic_bool complete, cancel;
static atomic_int tiles_processed;
static Texture2D tiles[12];
static char cache[4096], network_message[160]="Map not loaded. Load the visible area when online.";
static double last_request=-100;
static char summary_name[64];
static int summary_status;
static bool failure_form;
static int failure_package=-1;
static bool resume_confirm;
static int checkpoint_status;
static char checkpoint_path[4096];
static const Rectangle MAP={316,168,936,702};
static double X(double lon) {return (lon+180)/360*16384;}
static double Y(double lat) {double r=lat*3.141592653589793/180;return (1-asinh(tan(r))/3.141592653589793)/2*16384;}
static Vector2 Project(GeoPoint point)
{
    return (Vector2){MAP.x+(float)((X(point.lon)-tile_x)*256)*MAP.width/1024,
                     MAP.y+(float)((Y(point.lat)-tile_y)*256)*MAP.height/768};
}
static void Text(const char *s,float x,float y,float size,Color color)
{DrawTextEx(size>=20?heading_font:font,s,(Vector2){x,y},fmaxf(size,14),0,color);}
static bool DrawButton(Rectangle rect,const char *label,bool enabled,bool primary)
{
    bool hover=enabled && CheckCollisionPointRec(GetMousePosition(),rect);
    DrawRectangleRounded(rect,0.12f,8,primary&&enabled?ACCENT:hover?(Color){36,58,70,255}:PANEL);
    DrawRectangleRoundedLinesEx(rect,0.12f,8,1,hover?ACCENT:BORDER);
    Text(label,rect.x+14,rect.y+12,16,!enabled?(Color){119,138,158,255}:primary?BG:INK);
    if(hover)SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static bool Button(Rectangle rect,const char *label,bool enabled) {return DrawButton(rect,label,enabled,false);}
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
    } else DispatchPlanFrom(request_packages,request_position,&response);
    atomic_store(&complete,true);return NULL;
}
static void Start(int kind)
{
    if(busy || GetTime()-last_request<5)return;
    RoundDrivePause(workspace);
    last_request=GetTime();job=kind;response=(DeliveryRoute){0};
    memcpy(request_packages,workspace->packages,sizeof(request_packages));
    request_position=workspace->position;request_revision=revision;
    atomic_store(&complete,false);atomic_store(&tiles_processed,0);
    if(pthread_create(&worker,NULL,Work,NULL)==0)busy=true;
    else snprintf(network_message,sizeof(network_message),"Could not start network request. Please retry.");
}
void DispatchUIInit(DeliveryRound *state)
{
    workspace=state;
    DispatchUIReset();tile_x=(int)floor(X(-7.113))-2;tile_y=(int)floor(Y(52.256))-1;
    snprintf(cache,sizeof(cache),"%smap-cache",GetApplicationDirectory());
    if(mkdir(cache,0700)!=0 && !DirectoryExists(cache))
        snprintf(network_message,sizeof(network_message),"Map cache is not writable. Routing is still available.");
    font=ParcelFont(false);heading_font=ParcelFont(true);
    custom_font=font.texture.id!=GetFontDefault().texture.id;
}
void DispatchUISetViewport(float scale, Vector2 offset)
{
    ui_scale=scale;ui_offset=offset;
}
void DispatchUIReset(void)
{
    RoundReset(workspace);DashboardClearExportStatus();++revision;
    summary_status=0;summary_name[0]=0;
    failure_form=false;failure_package=-1;
    resume_confirm=false;checkpoint_status=0;
}
void DispatchUIUpdate(void)
{
    if(!busy || !atomic_load(&complete))return;
    pthread_join(worker,NULL);busy=false;
    if(job==1) {
        snprintf(network_message,sizeof(network_message),"%s",response.message);
        for(int i=0;i<12;++i) {
            if(tiles[i].id)UnloadTexture(tiles[i]);
            char path[4096];TilePath(path,sizeof(path),i);
            tiles[i]=FileExists(path)?LoadTexture(path):(Texture2D){0};
        }
    } else if(request_revision==revision) {
        workspace->route=response;
        workspace->run=(DeliveryRun){.speed=workspace->run.speed};
        if(workspace->route.ready && workspace->route.count)RoundSelect(workspace,workspace->route.order[0]);
    }
}

int DispatchUIDraw(void)
{
    Package *packages=workspace->packages;
    DeliveryRoute *route=&workspace->route;
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);ClearBackground(BG);
    DrawRectangle(0,0,288,1040,PANEL);
    DrawRectangleRounded((Rectangle){24,28,32,32},0.2f,8,ACCENT);
    DrawRectangleLinesEx((Rectangle){32,36,16,17},2,BG);
    Text("Parcel Ops",70,30,24,INK);
    Text("Delivery control",316,28,32,INK);
    Text(TextFormat("%s / %s",packages[workspace->selected].id,packages[workspace->selected].destination),316,76,17,MUTED);
    if(Button((Rectangle){1020,28,104,40},"Profile",true))return 2;
    if(Button((Rectangle){1136,28,116,40},"Lock",true))return 3;
    Text("Your delivery round",24,116,22,INK);
    int delivered=0,issues=RoundIssueCount(workspace);
    for(int i=0;i<5;++i) {
        delivered+=packages[i].delivered;
    }
    Text(TextFormat("%d of 5 delivered / %d flagged",delivered,issues),24,156,14,MUTED);
    if(IsKeyPressed(KEY_DOWN))RoundSelect(workspace,(workspace->selected+1)%5);
    if(IsKeyPressed(KEY_UP))RoundSelect(workspace,(workspace->selected+4)%5);
    if(!busy && IsKeyPressed(KEY_SPACE))RoundSetIncluded(workspace,workspace->selected,!packages[workspace->selected].included);
    for(int i=0;i<5;++i) {
        float y=198+i*100;Rectangle row={20,y,248,88},check={32,y+15,20,20};
        bool selected=i==workspace->selected;
        DrawRectangleRounded(row,0.12f,8,selected?(Color){29,59,63,255}:BG);
        if(selected)DrawRectangleRoundedLinesEx(row,0.12f,8,1,ACCENT);
        Text(packages[i].id,66,y+10,20,INK);
        Text(packages[i].destination,32,y+38,15,MUTED);
        Sensors *s=&workspace->sensors[i];
        Color condition=RoundPackageIssues(workspace,i)?(Color){255,185,112,255}:ACCENT;
        Text(packages[i].delivered?"Delivered / history retained":workspace->deferred[i]?"Deferred / retry required":TextFormat("%.1f C / %u impacts / %s",s->temperature_c,s->impact_count,s->connected?"live":"stale"),32,y+65,12,condition);
        DrawRectangleLinesEx(check,1.5f,packages[i].included?ACCENT:MUTED);
        if(packages[i].included)DrawRectangle(36,(int)y+19,12,12,ACCENT);
        if(route->ready)for(int j=0;j<route->count;++j)if(route->order[j]==i)Text(TextFormat("Stop %d",j+1),206,y+14,11,ACCENT);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),row)) {
            RoundSelect(workspace,i);
            if(!busy && CheckCollisionPointRec(GetMousePosition(),check))RoundSetIncluded(workspace,i,!packages[i].included);
        }
    }
    bool next=RoundCanAttempt(workspace);
    if(DrawButton((Rectangle){24,724,240,44},"Complete selected stop",!busy&&next,next)) {
        if(RoundDeliverNext(workspace))++revision;
    }
    bool deferred=workspace->deferred[workspace->selected];
    if(Button((Rectangle){24,784,240,44},deferred?"Retry selected parcel":"Report failed stop",!busy&&(deferred||next))) {
        if(deferred) { if(RoundRetry(workspace,workspace->selected))++revision; }
        else {failure_form=true;failure_package=workspace->selected;}
    }
    if(Button((Rectangle){24,844,240,44},"Next parcel to check (N)",issues>0) || (issues>0&&IsKeyPressed(KEY_N))) {
        RoundSelectNextIssue(workspace);return 1;
    }
    Text(issues?"Opens flagged parcel monitoring":"No active condition flags",24,906,12,MUTED);
    if(Button((Rectangle){24,940,240,44},"Reset delivery round",!busy))DispatchUIReset();
    Text("Simulated data / no live GPS",24,1008,12,MUTED);
    bool cooldown=GetTime()-last_request>=5;
    if(Button((Rectangle){316,110,188,40},"Load map (L)",!busy&&cooldown)||IsKeyPressed(KEY_L))Start(1);
    if(Button((Rectangle){516,110,232,40},"Plan driving route (P)",!busy&&cooldown)||IsKeyPressed(KEY_P))Start(2);
    int path_length=snprintf(checkpoint_path,sizeof(checkpoint_path),"%sparcel-round-v1.save",GetApplicationDirectory());
    bool checkpoint_available=path_length>0 && (size_t)path_length<sizeof(checkpoint_path);
    if(Button((Rectangle){772,110,112,40},"Save (S)",!busy&&checkpoint_available) || (!busy&&checkpoint_available&&IsKeyPressed(KEY_S))) {
        checkpoint_status=RoundSave(workspace,checkpoint_path)?1:-1;summary_status=0;resume_confirm=false;
    }
    if(Button((Rectangle){896,110,112,40},"Resume (O)",!busy&&checkpoint_available) || (!busy&&checkpoint_available&&IsKeyPressed(KEY_O))) {
        summary_status=0;failure_form=false;
        if(FileExists(checkpoint_path))resume_confirm=true;
        else checkpoint_status=-3;
    }
    if(Button((Rectangle){1020,110,232,40},"Export round report (R)",true)||IsKeyPressed(KEY_R))
        summary_status=RoundExportSummaryCSV(workspace,GetApplicationDirectory(),summary_name,sizeof(summary_name))?1:-1;
    if(Button((Rectangle){800,28,204,40},"Monitor selected parcel",true))return 1;
    BeginScissorMode((int)(MAP.x*ui_scale+ui_offset.x),(int)(MAP.y*ui_scale+ui_offset.y),
                     (int)(MAP.width*ui_scale),(int)(MAP.height*ui_scale));
    DrawRectangleRec(MAP,PANEL);
    for(int i=0;i<12;++i) {
        Rectangle dest={MAP.x+(i%4)*MAP.width/4,MAP.y+(i/4)*MAP.height/3,MAP.width/4,MAP.height/3};
        if(tiles[i].id)DrawTexturePro(tiles[i],(Rectangle){0,0,256,256},dest,(Vector2){0},0,WHITE);
        else DrawRectangleLinesEx(dest,1,(Color){43,57,69,255});
    }
    if(route->ready)for(int i=1;i<route->point_count;++i)
        DrawLineEx(Project(route->points[i-1]),Project(route->points[i]),4,(Color){29,86,207,255});
    for(int i=0;i<5;++i) {
        Vector2 pos=Project(packages[i].point);
        Color marker=packages[i].delivered?DARKGREEN:RoundPackageIssues(workspace,i)?(Color){161,59,25,255}:packages[i].included?(Color){29,86,207,255}:GRAY;
        DrawCircleV(pos,i==workspace->selected?14:11,marker);
        if(i==workspace->selected)DrawCircleLines((int)pos.x,(int)pos.y,17,WHITE);
        Text(TextFormat("%d",i+1),pos.x-5,pos.y-7,14,WHITE);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&!CheckCollisionPointRec(GetMousePosition(),(Rectangle){340,192,888,160})&&CheckCollisionPointCircle(GetMousePosition(),pos,18))RoundSelect(workspace,i);
    }
    Vector2 driver=Project(workspace->position);DrawCircleV(driver,13,BG);DrawCircleV(driver,9,ACCENT);
    DrawCircleLines((int)driver.x,(int)driver.y,14,WHITE);
    EndScissorMode();
    DrawRectangleRounded((Rectangle){340,192,888,160},0.14f,10,PANEL);
    DrawRectangleRoundedLinesEx((Rectangle){340,192,888,160},0.14f,10,1,BORDER);
    bool planned=route->ready && route->count>0;
    const char *run_state=!planned?"Plan a delivery round":workspace->run.state==DRIVE_RUNNING?"Driving to next stop":
        workspace->run.state==DRIVE_PAUSED?"Driving paused":workspace->run.state==DRIVE_ARRIVED?"Arrived at delivery stop":"Ready to depart";
    Text(run_state,364,210,24,INK);
    if(planned) {
        Package *target=&packages[route->order[0]];
        Text(TextFormat("%s / %s",target->id,target->destination),364,250,18,INK);
        double duration=route->leg_seconds[0];
        double progress=workspace->run.state==DRIVE_ARRIVED?1:duration>0?workspace->run.elapsed_seconds/duration:0;
        Text(TextFormat("%.0f%% of leg / %.1f km left / %.0f min road estimate",progress*100,
             route->leg_km[0]*(1-progress),fmax(0,duration-workspace->run.elapsed_seconds)/60),364,284,15,MUTED);
        DrawRectangleRounded((Rectangle){364,322,500,6},1,8,BORDER);
        if(progress>0)DrawRectangleRounded((Rectangle){364,322,(float)(500*progress),6},1,8,ACCENT);
    } else Text("Choose parcels, load the map and plan a driving route.",364,254,17,MUTED);
    bool driving=workspace->run.state==DRIVE_RUNNING;
    bool arrived=workspace->run.state==DRIVE_ARRIVED;
    bool drive_action=DrawButton((Rectangle){896,212,308,48},arrived?"Select arrived parcel":driving?"Pause drive (D)":"Start / resume drive (D)",planned&&!busy,!arrived);
    if((drive_action || IsKeyPressed(KEY_D)) && planned && !busy) {
        if(arrived)RoundSelect(workspace,route->order[0]);
        else if(driving)RoundDrivePause(workspace);
        else RoundDriveStart(workspace);
    }
    const int speeds[]={1,5,20};
    for(int i=0;i<3;++i) {
        Rectangle speed_button={896+i*104,280,96,40};
        if(Button(speed_button,TextFormat("%dx demo",speeds[i]),true))RoundDriveSpeed(workspace,speeds[i]);
        if(workspace->run.speed==speeds[i])DrawRectangleRoundedLinesEx(speed_button,0.12f,8,2,ACCENT);
    }
    DrawRectangle(316,850,936,20,PANEL);
    Text("Map data (c) OpenStreetMap contributors / ODbL",326,854,12,INK);
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(GetMousePosition(),(Rectangle){316,850,936,20}))
        OpenURL("https://www.openstreetmap.org/copyright");
    if(failure_package!=workspace->selected || !next || busy)failure_form=false;
    if(resume_confirm && !busy) {
        Text("Resume saved round and replace current progress?",316,900,20,INK);
        if(Button((Rectangle){316,944,304,40},"Resume saved round",true)) {
            bool loaded=RoundLoad(workspace,checkpoint_path);
            checkpoint_status=loaded?2:-2;resume_confirm=false;
            if(loaded) {++revision;DashboardClearExportStatus();failure_form=false;failure_package=-1;}
        }
        if(Button((Rectangle){632,944,304,40},"Keep current round",true))resume_confirm=false;
    } else if(failure_form) {
        Text(TextFormat("Record failed attempt for %s",packages[workspace->selected].id),316,900,20,INK);
        if(Button((Rectangle){1132,888,120,40},"Cancel",true))failure_form=false;
        const char *reasons[]={"Recipient absent","No access","Address problem"};
        for(int i=0;i<3;++i) {
            if(Button((Rectangle){316+i*316,944,304,40},reasons[i],true)) {
                if(RoundFailNext(workspace,(AttemptOutcome)(i+1)))++revision;
                failure_form=false;
            }
        }
    } else if(busy) {
        Text(job==1?TextFormat("Loading map tiles %d / 12",atomic_load(&tiles_processed)):"Planning road route...",316,900,20,ACCENT);
    } else if(route->ready) {
        Text(TextFormat("%.2f km / %.0f min estimated / %d stops",route->distance_km,route->duration_seconds/60,route->count),316,900,24,INK);
        char order[140]="Driver";
        for(int i=0;i<route->count;++i)strcat(order,TextFormat(" > %s",packages[route->order[i]].id));
        Text(order,316,944,14,MUTED);
    } else {
        Text(route->message[0]?route->message:"Select packages, then plan a route on real roads.",316,902,16,MUTED);
        Text(network_message,316,944,12,MUTED);
    }

    if(checkpoint_status && !summary_status) {
        const char *messages[]={"","Round saved beside executable / parcel-round-v1.save","Round resumed / elapsed time preserved / plan a fresh route"};
        Text(checkpoint_status>0?messages[checkpoint_status]:checkpoint_status==-1?
             "Save failed. Earlier snapshot preserved; check folder access.":checkpoint_status==-3?
             "No saved round found. Choose Save to create a checkpoint.":
             "Resume failed: missing, invalid or unsupported file. Current round unchanged.",316,1008,13,checkpoint_status>0?ACCENT:(Color){255,185,112,255});
    }
    if(summary_status)Text(summary_status>0?TextFormat("Saved %s beside the app executable",summary_name):
                          "Report export failed. Check folder access and available filenames.",316,1008,13,summary_status>0?ACCENT:(Color){255,185,112,255});
    return 0;
}
void DispatchUIClose(void)
{
    atomic_store(&cancel,true);if(busy)pthread_join(worker,NULL);
    for(int i=0;i<12;++i)if(tiles[i].id)UnloadTexture(tiles[i]);
    if(custom_font)UnloadFont(font);
    if(heading_font.texture.id!=GetFontDefault().texture.id)UnloadFont(heading_font);
}

#include "../src/round.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static DeliveryRound trip;
int main(void)
{
    RoundReset(&trip);assert(!RoundDriveStart(&trip));
    trip.route.ready=true;trip.route.count=2;trip.route.leg_count=2;trip.route.point_count=5;
    trip.route.order[0]=0;trip.route.order[1]=1;
    trip.route.points[0]=(GeoPoint){0,0};trip.route.points[1]=(GeoPoint){0,.001};
    trip.route.points[2]=(GeoPoint){0,.002};trip.route.points[3]=(GeoPoint){0,.002};
    trip.route.points[4]=(GeoPoint){0,.004};
    trip.route.leg_end[0]=2;trip.route.leg_end[1]=4;
    trip.route.leg_seconds[0]=10;trip.route.leg_seconds[1]=20;
    trip.route.leg_km[0]=.222;trip.route.leg_km[1]=.222;
    trip.route.duration_seconds=30;trip.route.distance_km=.444;
    trip.packages[0].point=trip.route.points[2];trip.packages[1].point=trip.route.points[4];
    assert(!RoundDeliverNext(&trip) && !RoundFailNext(&trip,ATTEMPT_NO_ACCESS));
    assert(RoundDriveStart(&trip));RoundUpdate(&trip,5);
    assert(trip.run.state==DRIVE_RUNNING && fabs(trip.position.lon-.001)<1e-8);
    RoundDrivePause(&trip);RoundUpdate(&trip,5);
    assert(trip.run.state==DRIVE_PAUSED && fabs(trip.position.lon-.001)<1e-8);
    assert(trip.sensors[0].seconds==10); /* Sensors do not pause with the driver. */
    assert(!RoundDriveSpeed(&trip,0) && !RoundDriveSpeed(&trip,100));
    assert(RoundDriveSpeed(&trip,20) && RoundDriveStart(&trip));RoundUpdate(&trip,100);
    assert(trip.run.state==DRIVE_ARRIVED && trip.route.count==2);
    assert(!RoundDriveStart(&trip));
    assert(fabs(trip.position.lon-.002)<1e-8); /* Never skips an unhandled stop. */
    assert(RoundFailNext(&trip,ATTEMPT_NO_ACCESS));
    assert(trip.deferred[0] && trip.route.ready && trip.route.count==1 && trip.selected==1);
    assert(trip.route.order[0]==1 && trip.route.leg_end[0]==2 && trip.route.leg_seconds[0]==20);
    assert(RoundDriveStart(&trip));RoundUpdate(&trip,NAN);
    assert(trip.run.elapsed_seconds==0);
    RoundUpdate(&trip,1);assert(trip.run.state==DRIVE_ARRIVED);
    assert(RoundDeliverNext(&trip) && !trip.route.ready && trip.packages[1].delivered);
    assert(trip.attempt_count[0]==1 && trip.attempt_count[1]==1);
    assert(RoundRetry(&trip,0) && trip.packages[0].included);
    RoundReset(&trip);assert(trip.run.state==DRIVE_READY && trip.run.speed==1);
    trip.route.ready=true;trip.route.count=1;trip.route.point_count=2;
    assert(!RoundCanAttempt(&trip) && !RoundDriveStart(&trip)); /* Missing leg metadata cannot bypass arrival. */
    puts("Delivery run checks passed: road interpolation, pause, speed, arrival gating, failed stops and remaining legs.");
}

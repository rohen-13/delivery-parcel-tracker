#include "../src/dispatch.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv)
{
    assert(DispatchLogin("demo", "parcel-demo"));
    assert(!DispatchLogin("demo", "wrong"));
    assert(!DispatchLogin(NULL, "parcel-demo"));
    double d[6][6] = {{0,9,1},{2,0,8},{8,1,0}};
    int order[5]; double km;
    assert(DispatchBestOrder((const double (*)[6])d, 2, order, &km));
    assert(order[0] == 1 && order[1] == 0 && km == 2);
    d[0][2] = NAN;
    assert(DispatchBestOrder((const double (*)[6])d, 2, order, &km));
    assert(order[0] == 0 && km == 17);
    d[0][1] = -1;
    assert(!DispatchBestOrder((const double (*)[6])d, 2, order, &km));
    assert(!DispatchBestOrder((const double (*)[6])d, 0, order, &km));
    assert(!DispatchParseMatrix("{}",2,d));
    assert(!DispatchParseMatrix("{",2,d));
    const char *bad[]={"\"","{\"a\"","{\"a\":\"\\u","[1,","{\"a\":1","{\"a\":\"\\"};
    for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);++i)
        assert(!DispatchParseMatrix(bad[i],2,d));
    assert(DispatchParseMatrix("{\"sources_to_targets\":[[{\"distance\":0},{\"distance\":2}],[{\"distance\":3},{\"distance\":0}]]}",1,d));
    assert(d[0][1] == 2 && d[1][0] == 3);
    DeliveryRoute r = {0};
    assert(!DispatchParseRoute("{\"error\":\"No path\"}",&r));
    assert(!DispatchParseRoute("{\"trip\":{\"summary\":{\"length\":1,\"time\":2},\"legs\":[]}}",&r));
    assert(DispatchParseRoute("{\"trip\":{\"summary\":{\"length\":1,\"time\":2},\"legs\":[{\"shape\":\"_p~iF~ps|U_ulLnnqC_mqNvxq`@\"}]}}",&r));
    assert(r.ready && r.point_count==3 && r.distance_km==1);
    assert(fabs(r.points[0].lat-3.85)<0.000001);
    assert(!DispatchParseRoute("{\"trip\":{\"summary\":{\"length\":1,\"time\":2},\"legs\":[{\"shape\":\"_\"}]}}",&r));
    assert(!r.ready && r.point_count==0);
    Package p[5]; DispatchSeed(p);
    assert(p[0].included && !p[0].delivered && strlen(p[0].id)>0);
    for(int i=0;i<5;++i)p[i].delivered=true;
    assert(!DispatchPlan(p,&r) && !r.ready);
    DispatchSeed(p);
    if(argc==2 && strcmp(argv[1],"--live")==0) {
        assert(DispatchPlan(p,&r));
        assert(r.ready && r.count==5 && r.point_count>5 && r.distance_km>0);
        printf("Live route: %.3f km, %.0f seconds, %d road points; order",r.distance_km,r.duration_seconds,r.point_count);
        for(int i=0;i<r.count;++i)printf(" %d",r.order[i]+1);
        puts("");
    }
    puts("Dispatch checks passed: demo login, asymmetric distance optimisation and response validation.");
}

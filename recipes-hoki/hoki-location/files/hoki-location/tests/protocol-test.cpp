#include "locationprotocol.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 HokiReport r;
 CHECK(parseHokiReport("HOKI1 POSITION 63 1789900000000 52.5 13.4 35 2.5 90 4 8",r));
 CHECK(r.kind==HokiReport::Position && r.location.timestamp()==1789900000000LL);
 CHECK(r.location.latitude()==52.5 && std::abs(r.location.speed()*1852.0/3600.0-2.5)<1e-9 && r.location.accuracy().vertical()==8);
 CHECK(parseHokiReport("HOKI1 POSITION 1 1789900000000 52.5 13.4 0 0 0 0 0",r));
 CHECK(std::isnan(r.location.altitude()) && std::isnan(r.location.accuracy().horizontal()));
 CHECK(!parseHokiReport("HOKI1 POSITION 0 1789900000000 52.5 13.4 0 0 0 0 0",r));
 CHECK(!parseHokiReport("HOKI1 POSITION 1 0 52.5 13.4 0 0 0 0 0",r));
 CHECK(!parseHokiReport("HOKI1 POSITION 1 1789900000000 91 13.4 0 0 0 0 0",r));
 CHECK(!parseHokiReport("HOKI1 POSITION 1 1789900000000 nan 13.4 0 0 0 0 0",r));
 CHECK(parseHokiReport("HOKI1 SV 3 14 3 0 40 284 10 195 4 25 30 119 4 2 6 31 47 198 4",r));
 CHECK(r.satellites.size()==3 && r.satellites[0].prn()==78 && r.satellites[0].snr()==0);
 CHECK(r.satellites[1].prn()==195 && r.satellites[2].prn()==302);
 CHECK(r.used==QList<int>({195,302}));
 CHECK(parseHokiReport("HOKI1 SV 0",r) && r.satellites.isEmpty());
 CHECK(!parseHokiReport("HOKI1 SV 177",r));
 CHECK(!parseHokiReport("HOKI1 SV 1 1 1 30 45",r));
 CHECK(!parseHokiReport("HOKI1 SV 1 1 1 30 999999999999 10 0",r));
 CHECK(!parseHokiReport("unexpected vendor text",r));
 puts("protocol tests passed");
}

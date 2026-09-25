// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include <QList>
#include "locationtypes.h"
#include <QByteArray>
#include <QList>
#include <cmath>
struct HokiReport {
    enum Kind {Position, Satellite} kind;
    Location location;
    QList<SatelliteInfo> satellites;
    QList<int> used;
};
inline bool parseHokiReport(const QByteArray &line,HokiReport &report) {
    if(!line.startsWith("HOKI1 "))return false;
    auto fields=line.split(' ');if(fields.size()<2)return false;
    if(fields[1]=="POSITION" && fields.size()==11){
        bool ok;unsigned flags=fields[2].toUInt(&ok);if(!ok||!(flags&1))return false;
        qint64 timestamp=fields[3].toLongLong(&ok);if(!ok||timestamp<=0)return false;
        double v[7];for(int i=0;i<7;i++){v[i]=fields[4+i].toDouble(&ok);if(!ok)return false;}
        if(!std::isfinite(v[0])||!std::isfinite(v[1])||std::abs(v[0])>90||std::abs(v[1])>180)return false;
        Location loc;loc.setTimestamp(timestamp);loc.setLatitude(v[0]);loc.setLongitude(v[1]);
        if((flags&2)&&std::isfinite(v[2]))loc.setAltitude(v[2]);
        if((flags&4)&&std::isfinite(v[3])&&v[3]>=0)loc.setSpeed(v[3]*3600.0/1852.0);
        if((flags&8)&&std::isfinite(v[4])&&v[4]>=0&&v[4]<=360)loc.setDirection(v[4]);
        Accuracy accuracy;if((flags&16)&&std::isfinite(v[5])&&v[5]>=0)accuracy.setHorizontal(v[5]);
        if((flags&32)&&std::isfinite(v[6])&&v[6]>=0)accuracy.setVertical(v[6]);loc.setAccuracy(accuracy);
        report.location=loc;report.kind=HokiReport::Position;return true;
    }else if(fields[1]=="SV" && fields.size()>=3){
        bool ok;unsigned count=fields[2].toUInt(&ok);if(!ok||count>176||fields.size()!=3+int(count)*6)return false;
        QList<SatelliteInfo> sats;QList<int> used;
        for(unsigned i=0;i<count;i++){
            int o=3+i*6;int prn=fields[o].toInt(&ok);if(!ok||prn<=0||prn>1000)return false;
            int type=fields[o+1].toInt(&ok);if(!ok||type<1||type>6)continue;
            double snr=fields[o+2].toDouble(&ok);if(!ok||!std::isfinite(snr)||snr<0||snr>100)return false;
            double elevation=fields[o+3].toDouble(&ok);if(!ok||!std::isfinite(elevation)||elevation < -90||elevation>90)return false;
            double azimuth=fields[o+4].toDouble(&ok);if(!ok||!std::isfinite(azimuth)||azimuth<0||azimuth>360)return false;
            unsigned flags=fields[o+5].toUInt(&ok);if(!ok)return false;
            if(type==2)prn-=87;else if(type==3 && prn<65)prn+=64;else if(type==5)prn+=200;else if(type==6)prn+=300;
            SatelliteInfo sat;sat.setPrn(prn);sat.setSnr(int(snr));sat.setElevation(int(elevation));sat.setAzimuth(int(azimuth));sats.append(sat);if(flags&4)used.append(prn);
        }
        report.satellites=sats;report.used=used;report.kind=HokiReport::Satellite;return true;
    }
    return false;
}

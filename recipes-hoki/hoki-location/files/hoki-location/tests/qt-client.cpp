#include <QCoreApplication>
#include <QGeoPositionInfoSource>
#include <QGeoSatelliteInfoSource>
#include <QGeoSatelliteInfo>
#include <QTimer>
#include <QDebug>
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);
 auto pos=QGeoPositionInfoSource::createDefaultSource(&app);
 auto sat=QGeoSatelliteInfoSource::createDefaultSource(&app);
 if(!pos||!sat){qCritical()<<"Missing QtPositioning source";return 2;}
 qInfo()<<"QT_SOURCES"<<pos->sourceName()<<sat->sourceName();
 int reports=0;
 QObject::connect(pos,&QGeoPositionInfoSource::positionUpdated,&app,[](const QGeoPositionInfo &p){qInfo()<<"QT_POSITION_VALID"<<p.isValid();});
 QObject::connect(sat,&QGeoSatelliteInfoSource::satellitesInViewUpdated,&app,[&](const QList<QGeoSatelliteInfo>& list){
   int positiveSignals=0;for(auto &s:list)if(s.signalStrength()>0)++positiveSignals;++reports;qInfo()<<"QT_SATELLITES"<<list.size()<<"signals"<<positiveSignals;
 });
 QObject::connect(pos,&QGeoPositionInfoSource::errorOccurred,&app,[](auto error){qWarning()<<"QT_POSITION_ERROR"<<error;});
 QObject::connect(sat,&QGeoSatelliteInfoSource::errorOccurred,&app,[](auto error){qWarning()<<"QT_SATELLITE_ERROR"<<error;});
 pos->setUpdateInterval(2000);sat->setUpdateInterval(2000);pos->startUpdates();sat->startUpdates();
 QTimer::singleShot(20000,&app,&QCoreApplication::quit);app.exec();pos->stopUpdates();sat->stopUpdates();delete pos;delete sat;
 return reports>0?0:3;
}

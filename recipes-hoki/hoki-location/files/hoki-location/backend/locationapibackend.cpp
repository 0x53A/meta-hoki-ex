// SPDX-License-Identifier: LGPL-2.1-or-later
#include "locationapibackend.h"
#include "locationprotocol.h"
#include "hybrisprovider.h"
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QDateTime>
#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDebug>
#include <cmath>

static const QString helper=QStringLiteral("/usr/libexec/hoki-location-helper");
static const QString assistance=QStringLiteral("/usr/libexec/hoki-location-assist");
static QProcessEnvironment vendorEnvironment() {
    auto env=QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LD_LIBRARY_PATH"),QStringLiteral("/vendor/lib:/system/lib"));
    return env;
}
LocationApiBackend::LocationApiBackend() : m_cache(QStringLiteral("/var/cache/hoki-location/xtra.bin")) {
    m_process.setProcessEnvironment(vendorEnvironment());
    m_assist.setProcessEnvironment(vendorEnvironment());
    m_process.setProcessChannelMode(QProcess::SeparateChannels);
    for(auto t:{&m_deadline,&m_retry,&m_assistDeadline,&m_assistRetry})t->setSingleShot(true);
    connect(&m_process,&QProcess::readyReadStandardOutput,this,&LocationApiBackend::readOutput);
    // Vendor diagnostics may include locations; consume but never forward them to the journal.
    connect(&m_process,&QProcess::readyReadStandardError,this,[this]{m_process.readAllStandardError();});
    connect(&m_deadline,&QTimer::timeout,this,[this]{qWarning()<<"LocationAPI command timed out";m_process.kill();});
    connect(&m_retry,&QTimer::timeout,this,&LocationApiBackend::launch);
    connect(&m_process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){if(e==QProcess::FailedToStart)failed();});
    connect(&m_process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus){
        readOutput();qInfo()<<"LocationAPI helper exited"<<code;failed();
    });
    connect(&m_assist,&QProcess::readyReadStandardOutput,this,[this]{
        m_assist.readAllStandardOutput(); // Diagnostics are unused; drain without accumulating or truncating valid transfers.
    });
    connect(&m_assist,&QProcess::readyReadStandardError,this,[this]{m_assist.readAllStandardError();});
    connect(&m_assistDeadline,&QTimer::timeout,this,[this]{m_assist.kill();});
    connect(&m_assistRetry,&QTimer::timeout,this,&LocationApiBackend::assistOrbit);
    connect(&m_assist,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus exit){
        m_assistDeadline.stop();int phase=m_assistPhase;m_assistPhase=0;
        if(!phase)return; // Cancellation is not a failed orbit injection.
        bool ok=exit==QProcess::NormalExit && code==0;
        qInfo()<<"GNSS assistance"<<(phase==1?"UTC":"orbit")<<(ok?"accepted":"failed");
        if(!m_wanted||!m_ready)return;
        if(phase==1)assistOrbit();
        else if(phase==2 && !ok && m_orbitAttempts<2){
            // One refreshed retry; never reuse a rejected cache as accepted assistance.
            QFile::remove(m_cache);m_assistRetry.start(10000);
        }
    });
    connect(&m_assist,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){
        if(e==QProcess::FailedToStart){m_assistDeadline.stop();int phase=m_assistPhase;m_assistPhase=0;qWarning()<<"Unable to start assistance helper";if(phase==1)assistOrbit();}
    });
}
LocationApiBackend::~LocationApiBackend(){gnssCleanup();}
bool LocationApiBackend::gnssInit(){return QFileInfo(helper).isExecutable()&&QFileInfo(assistance).isExecutable();}
// QProcess notifications run in the provider thread: deliver in order, without
// leaving old fixes/status queued across a stop or helper restart.
void LocationApiBackend::status(int value){QMetaObject::invokeMethod(staticProvider,"backendStatus",Qt::AutoConnection,Q_ARG(int,value));}
bool LocationApiBackend::gnssStart(){
    if(m_wanted)return true;
    m_wanted=true;m_attempt=0;m_orbitAttempts=0;m_downloadAttempts=0;++m_generation;
    if(m_process.state()==QProcess::NotRunning)launch();
    return true;
}
void LocationApiBackend::launch(){
    if(!m_wanted||m_process.state()!=QProcess::NotRunning)return;
    ++m_attempt;m_ready=false;m_stopping=false;m_updating=false;m_buffer.clear();
    m_sentInterval=m_interval;m_process.start(helper,{QString::number(m_interval)});
    m_deadline.start(45000);status(HybrisProvider::StatusAcquiring);
}
void LocationApiBackend::failed(){
    ++m_generation; // Invalidate timedate replies and delayed assistance from the old process.
    m_deadline.stop();m_ready=false;m_updating=false;cancelAssistance();
    if(m_wanted){
        if(m_stopping){m_stopping=false;m_retry.start(0);}
        else if(m_attempt<3){status(HybrisProvider::StatusAcquiring);m_retry.start(2000*m_attempt);}
        else {qWarning()<<"LocationAPI recovery exhausted; restart client to retry";status(HybrisProvider::StatusError);}
    }
}
bool LocationApiBackend::gnssStop(){
    m_wanted=false;++m_generation;m_ready=false;m_retry.stop();cancelAssistance();
    if(m_process.state()!=QProcess::NotRunning){m_stopping=true;m_process.write("STOP\n");m_process.closeWriteChannel();m_deadline.start(35000);}
    return true;
}
void LocationApiBackend::gnssCleanup(){
    gnssStop();if(m_process.state()!=QProcess::NotRunning && !m_process.waitForFinished(32000)){m_process.kill();m_process.waitForFinished(1000);}
}
bool LocationApiBackend::gnssSetPositionMode(HybrisGnssPositionMode mode,HybrisGnssPositionRecurrence recurrence,uint32_t interval,uint32_t,uint32_t){
    if(mode!=HYBRIS_GNSS_POSITION_MODE_STANDALONE||recurrence!=HYBRIS_GNSS_POSITION_RECURRENCE_PERIODIC)return false;
    m_interval=qBound(1000u,interval,3600000u);updateInterval();return true;
}
void LocationApiBackend::updateInterval(){
    if(m_ready&&m_wanted&&!m_updating&&m_interval!=m_sentInterval){
        m_sentInterval=m_interval;m_updating=true;m_process.write("UPDATE "+QByteArray::number(m_interval)+"\n");m_deadline.start(15000);
    }
}
void LocationApiBackend::gnssDeleteAidingData(HybrisGnssAidingData){qWarning()<<"Deleting modem aiding data is not implemented";}
void LocationApiBackend::readOutput(){
    m_buffer+=m_process.readAllStandardOutput();
    if(m_buffer.size()>262144){m_process.kill();m_buffer.clear();return;}
    int end;while((end=m_buffer.indexOf('\n'))>=0){auto line=m_buffer.left(end);m_buffer.remove(0,end+1);parseLine(line);}
}
void LocationApiBackend::parseLine(const QByteArray &line){
    if(!line.startsWith("HOKI1 "))return;
    auto fields=line.split(' ');if(fields.size()<2)return;
    if(fields[1]=="READY"){
        if(!m_wanted||m_stopping)return;
        m_ready=true;m_deadline.stop();qInfo()<<"LocationAPI ready interval"<<m_sentInterval;updateInterval();
        QMetaObject::invokeMethod(staticProvider,"engineOn",Qt::AutoConnection);
        unsigned generation=m_generation;
        QTimer::singleShot(1000,this,[this,generation]{if(m_wanted&&m_ready&&generation==m_generation)assistTime();});return;
    }
    if(fields[1]=="UPDATED"){
        if(!m_wanted||!m_ready||m_stopping||!m_updating)return;
        if(fields.size()!=3||fields[2].toUInt()!=m_sentInterval){m_process.kill();return;}
        m_updating=false;m_deadline.stop();qInfo()<<"LocationAPI interval acknowledged"<<m_sentInterval;updateInterval();return;
    }
    if(!m_wanted||!m_ready)return;
    HokiReport report;
    if(!parseHokiReport(line,report))return;
    if(report.kind==HokiReport::Position)QMetaObject::invokeMethod(staticProvider,"setLocation",Qt::AutoConnection,Q_ARG(Location,report.location));
    else QMetaObject::invokeMethod(staticProvider,"setSatellite",Qt::AutoConnection,Q_ARG(QList<SatelliteInfo>,report.satellites),Q_ARG(QList<int>,report.used));
}

void LocationApiBackend::cancelAssistance(){
    m_assistRetry.stop();m_assistDeadline.stop();m_assistPhase=0;
    if(m_reply){auto reply=m_reply;m_reply=nullptr;reply->abort();reply->deleteLater();}
    if(m_assist.state()!=QProcess::NotRunning){m_assist.kill();m_assist.waitForFinished(1000);}
}
void LocationApiBackend::assistTime(){
    if(!m_wanted||!m_ready)return;
    auto message=QDBusMessage::createMethodCall("org.freedesktop.timedate1","/org/freedesktop/timedate1","org.freedesktop.DBus.Properties","Get");
    message<<QString("org.freedesktop.timedate1")<<QString("NTPSynchronized");
    auto watcher=new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message,2000),this);
    unsigned generation=m_generation;
    connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,watcher,generation]{
        QDBusPendingReply<QDBusVariant> result=*watcher;watcher->deleteLater();
        if(!m_wanted||!m_ready||generation!=m_generation)return;
        if(!result.isError()&&result.value().variant().toBool())runAssistance({"--inject-time"},1);
        else {qWarning()<<"GNSS UTC assistance skipped: system clock not confirmed synchronized";assistOrbit();}
    });
}
void LocationApiBackend::runAssistance(const QStringList &args,int phase){
    if(!m_wanted||!m_ready||m_assist.state()!=QProcess::NotRunning)return;
    m_assistPhase=phase;m_assist.start(assistance,args);m_assistDeadline.start(phase==1?20000:90000);
}
void LocationApiBackend::assistOrbit(){
    if(!m_wanted||!m_ready||m_assist.state()!=QProcess::NotRunning||m_reply)return;
    QFileInfo info(m_cache);qint64 age=info.lastModified().secsTo(QDateTime::currentDateTimeUtc());
    if(info.isFile()&&info.size()>0&&info.size()<=1048576&&age>=0&&age<86400){
        ++m_orbitAttempts;runAssistance({"--inject-xtra",m_cache},2);
    }else downloadOrbit();
}
void LocationApiBackend::downloadOrbit(){
    if(!m_wanted||!m_ready||m_reply||m_downloadAttempts>=2)return;
    ++m_downloadAttempts;
    QNetworkRequest req(QUrl(QStringLiteral("https://xtrapath1.izatcloud.net/xtra3grcej.bin")));
    req.setTransferTimeout(20000);req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::SameOriginRedirectPolicy);
    m_reply=m_network.get(req);auto reply=m_reply;reply->setReadBufferSize(1048577);
    connect(reply,&QNetworkReply::readyRead,this,[reply]{if(reply->bytesAvailable()>1048576)reply->abort();});
    connect(reply,&QNetworkReply::finished,this,[this,reply]{
        if(m_reply!=reply)return;m_reply=nullptr;reply->deleteLater();
        if(!m_wanted||!m_ready)return;
        bool ok=reply->error()==QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200;
        QByteArray data=reply->readAll();ok=ok&&!data.isEmpty()&&data.size()<=1048576;
        if(ok){QSaveFile file(m_cache);ok=file.open(QIODevice::WriteOnly)&&file.write(data)==data.size()&&file.commit();}
        if(ok)assistOrbit();else {qWarning()<<"GNSS orbit download/cache failed; acquisition continues";if(m_downloadAttempts<2)m_assistRetry.start(60000);}
    });
}

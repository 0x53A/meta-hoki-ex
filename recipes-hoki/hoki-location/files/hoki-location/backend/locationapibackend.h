// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include "hybrislocationbackend.h"
#include <QProcess>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QNetworkReply>

class LocationApiBackend : public HybrisLocationBackend {
    Q_OBJECT
public:
    LocationApiBackend();
    ~LocationApiBackend() override;
    bool gnssInit() override;
    bool gnssStart() override;
    bool gnssStop() override;
    void gnssCleanup() override;
    bool gnssSetPositionMode(HybrisGnssPositionMode,HybrisGnssPositionRecurrence,uint32_t,uint32_t,uint32_t) override;
    bool gnssInjectTime(HybrisGnssUtcTime,int64_t,int32_t) override { return false; }
    bool gnssInjectLocation(int,double,double,float) override { return false; }
    void gnssDeleteAidingData(HybrisGnssAidingData) override;
    void gnssDebugInit() override {}
    void gnssNiInit() override {}
    void gnssNiRespond(int32_t,HybrisGnssUserResponseType) override {}
    void gnssXtraInit() override {}
    bool gnssXtraInjectXtraData(QByteArray&) override { return false; }
    void aGnssInit() override {}
    bool aGnssDataConnClosed() override { return false; }
    bool aGnssDataConnFailed() override { return false; }
    bool aGnssDataConnOpen(const QByteArray&,const QString&) override { return false; }
    int aGnssSetServer(HybrisAGnssType,const char*,int) override { return 0; }
    void aGnssRilInit() override {}
private:
    friend class LocationApiBackendTest;
    void launch();
    void readOutput();
    void parseLine(const QByteArray&);
    void updateInterval();
    void status(int);
    void cancelAssistance();
    void assistTime();
    void assistOrbit();
    void downloadOrbit();
    void runAssistance(const QStringList&,int);
    void failed();
    QProcess m_process, m_assist;
    QTimer m_deadline, m_retry, m_assistDeadline, m_assistRetry;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QByteArray m_buffer;
    bool m_wanted=false, m_ready=false, m_stopping=false, m_updating=false;
    unsigned m_interval=1000, m_sentInterval=1000, m_attempt=0, m_generation=0;
    int m_assistPhase=0, m_orbitAttempts=0, m_downloadAttempts=0;
    QString m_cache;
};

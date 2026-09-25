// SPDX-License-Identifier: LGPL-2.1-or-later
#include <QCoreApplication>
#include "hybrisprovider.h"
#include "locationapibackend.h"
#include <cstdio>
HybrisProvider *staticProvider=nullptr;
class LocationApiBackendTest {
public:
    static int run() {
        LocationApiBackend b;
        b.m_stopping=true; b.m_wanted=false; b.m_ready=false;
        b.m_updating=true; b.m_sentInterval=5000; b.m_deadline.start(35000);
        b.parseLine("HOKI1 UPDATED 5000");
        if(!b.m_deadline.isActive()) return 1;
        // A new client can arrive while the previous helper is still stopping.
        b.m_wanted=true;
        b.parseLine("HOKI1 UPDATED 5000");
        if(!b.m_deadline.isActive()) return 2;
        b.m_stopping=false; b.m_ready=true;
        b.m_interval=b.m_sentInterval=5000; b.m_updating=true; b.m_deadline.start(15000);
        b.parseLine("HOKI1 UPDATED 5000");
        if(b.m_deadline.isActive()||b.m_updating) return 3;
        auto generation=b.m_generation;
        b.m_wanted=false; b.failed();
        if(b.m_generation==generation) return 4;
        b.m_assistPhase=2;
        b.m_assist.start("/bin/sh",{"-c","dd if=/dev/zero bs=1024 count=128 2>/dev/null; sleep 1"});
        if(!b.m_assist.waitForFinished(5000) || b.m_assist.exitStatus()!=QProcess::NormalExit || b.m_assist.exitCode()!=0)return 5;
        puts("PASS: assistance can produce more than 64 KiB of diagnostics without being killed");
        puts("PASS: late acknowledgements preserve shutdown deadline; helper failure invalidates async assistance");
        return 0;
    }
};
int main(int argc,char **argv) { QCoreApplication app(argc,argv); return LocationApiBackendTest::run(); }

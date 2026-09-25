#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDateTime>
#include <QVariantMap>
#include <QTimer>
#include <QDebug>
#include <unistd.h>
int main(int argc,char** argv){
 QCoreApplication app(argc,argv);if(argc<3)return 2;
 int seconds=QString::fromLocal8Bit(argv[1]).toInt(),interval=QString::fromLocal8Bit(argv[2]).toInt();
 bool crash=argc>3;
 auto bus=QDBusConnection::sessionBus();
 auto call=[&](const QString &iface,const QString &method,const QList<QVariant> &args=QList<QVariant>()){
  auto m=QDBusMessage::createMethodCall("org.freedesktop.Geoclue.Providers.Hybris","/org/freedesktop/Geoclue/Providers/Hybris",iface,method);m.setArguments(args);return bus.call(m,QDBus::Block,10000);
 };
 if(call("org.freedesktop.Geoclue","AddReference").type()==QDBusMessage::ErrorMessage)return 3;
 QVariantMap options;options["UpdateInterval"]=interval;
 if(call("org.freedesktop.Geoclue","SetOptions",{options}).type()==QDBusMessage::ErrorMessage)return 4;
 qInfo()<<"CLIENT_STARTED"<<getpid()<<"interval"<<interval;
 QTimer timer;QObject::connect(&timer,&QTimer::timeout,&app,[&]{
  auto s=call("org.freedesktop.Geoclue","GetStatus");auto v=call("org.freedesktop.Geoclue.Satellite","GetSatellite");auto p=call("org.freedesktop.Geoclue.Position","GetPosition");
  if(s.type()==QDBusMessage::ErrorMessage||v.type()==QDBusMessage::ErrorMessage||p.type()==QDBusMessage::ErrorMessage){qWarning()<<"DBUS_FAILURE";app.exit(5);return;}
  qInfo()<<"REPORT status"<<s.arguments()[0].toInt()<<"sat-time"<<v.arguments()[0].toInt()<<"used"<<v.arguments()[1].toInt()<<"listed"<<v.arguments()[2].toInt()<<"position-fields"<<p.arguments()[0].toInt();
 });timer.start(3000);
 QTimer::singleShot(seconds*1000,&app,[&]{if(crash){qInfo()<<"CLIENT_DISCONNECT";_exit(0);}app.quit();});
 int result=app.exec();auto r=call("org.freedesktop.Geoclue","RemoveReference");qInfo()<<"CLIENT_RELEASED";return r.type()==QDBusMessage::ErrorMessage?6:result;
}

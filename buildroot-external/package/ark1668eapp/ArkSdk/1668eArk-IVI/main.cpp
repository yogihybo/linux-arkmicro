#include "getopt.h"
#include "UserInterface/QmlLauncher.h"
#include "MultimediaService.h"
#include "AudioService.h"
#include "UserInterfaceUtility.h"
#include "UserInterface/Launcher.h"
#include "BusinessLogic/QmlWidget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Setting.h"
#include "ArkApplication.h"
//#include "BusinessLogic/Multimedia.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQmlContext>
#include <QResource>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <QTextCodec>
#include <QDebug>
#include <QtConvert.h>
static void setTextCodec();
QObject *QmlWidget_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return QmlWidget::instance();
}
int main(int argc, char *argv[])
{
    int result = getopt(argc, argv, "t:");
    if (-1 != result) {
        QString argument(optarg);
        if (!argument.isEmpty()) {
            switch(result) {
            case 't': {
                if (MultimediaApplication == argument) {
                    QCoreApplication app(argc, argv);
                    setTextCodec();
                    MultimediaService multimediaService;
                    qDebug()<<"######0000########";
                    (void)multimediaService;
                    return app.exec();
                }else if (AudioApplication == argument) {
                    nice(10);
                    QCoreApplication app(argc, argv);
                    qDebug()<<"######1111#######";
                    AudioService audioService;
                    (void)audioService;
                    return app.exec();
                } else {
                    return EXIT_FAILURE;
                }
                break;
            }
            default: {
                return EXIT_FAILURE;
                break;
            }
            }
        }
    }
#if defined(Q_OS_WIN)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    ArkApplication app(argc, argv);
    static const QString LanguageConfig("/data/Language.ini");
    QFile _LanguagecfgFile(LanguageConfig);
    int _LanguageType = 1;
    if(_LanguagecfgFile.exists()){
        QSettings *LanguagecfgsetFile = new QSettings(LanguageConfig,QSettings::IniFormat);
        _LanguageType = LanguagecfgsetFile->value("Language").toInt();
        delete LanguagecfgsetFile;
    }
    QString languagePath;
    languagePath.clear();
    switch (_LanguageType) {
        case LT_Chinese: {
            languagePath = QString(":/Languages/zh_tr.qm");
            break;
        }
        case LT_TChinese: {
            languagePath = QString(":/Languages/Tzh_tr.qm");
            break;
        }
        case LT_English:
        default: {
            languagePath = QString(":/Languages/en_tr.qm");
            break;
        }
    }
    ArkApp->installTranslatorPath(languagePath);
    setTextCodec();
    QString m_Rcc = "/usr/share/WSVGA.rcc";
    QResource::registerResource(m_Rcc);
    qmlRegisterSingletonType<QmlWidget>("com.QmlWidget.model", 1, 0, "QmlWidget",QmlWidget_provider);
    QmlWidget::instance();
    QQmlApplicationEngine* engine = new QQmlApplicationEngine(&app);
    QmlLauncher* m_QmlLauncher = new QmlLauncher(engine);
    m_QmlLauncher->qmlLauncherInit();
    //g_Audio->requestAudioSource(AS_Idle);
    engine->load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine->rootObjects().isEmpty())
        return -1;
    QObject* qmlObject   = engine->rootObjects().value(0);
    Launcher* launcher   = new Launcher(&app);
    launcher->setLauncherObject(qmlObject);
    return app.exec();
}

static void setTextCodec()
{
    QTextCodec *codec = QTextCodec::codecForName("utf-8");
    QTextCodec::setCodecForLocale(codec);
}

#ifndef MULTIMEDIAWIDGET_H
#define MULTIMEDIAWIDGET_H

#include <QObject>
enum UsbType{
    UsbUndefine,
    UsbNotConnect ,
    UsbScaning,
    UsbConnect
};
enum SdType{
    SdUndefine,
    SdNotConnect ,
    SdScaning,
    SdConnect
};
class MultiMediaWidgetPrivate;
class MultiMediaWidget : public QObject
{
    Q_OBJECT
public:
    explicit MultiMediaWidget(QObject *parent = nullptr);
    void setMultiMediaWidgetObject(QObject* qmlObject);
public slots:
    void onDeviceWatcherStatus(const int type, const int status);
    void onConnectStatusChange(const int status);
    void onMusicStatusChange(const QString& musicName, const int status);
private:
    MultiMediaWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(MultiMediaWidget)
};

#endif // MULTIMEDIAWIDGET_H

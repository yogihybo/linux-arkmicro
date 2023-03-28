#ifndef MUSICINFORMATIONWIDGET_H
#define MUSICINFORMATIONWIDGET_H

#include <QObject>
#include "ImageProvider.h"
class MusicInformationWidgetPrivate;
class MusicInformationWidget : public QObject
{
    Q_OBJECT
public:
    explicit MusicInformationWidget(QObject *parent = nullptr);
    ImageProvider* getImageProvider();
    Q_INVOKABLE bool isNullConverImage();
signals:
    void callQmlRefreshImg();
protected slots:
    void onMusicPlayerID3TagChange(const int type,
                                   const int index,
                                   const QString &fileName,
                                   const QString& title,
                                   const QString& artist,
                                   const QString& album,
                                   const int endTime);
    void onUsbMediaPlayExit();
    void onSdMediaPlayExit();
private:
    MusicInformationWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(MusicInformationWidget)
};

#endif // MUSICINFORMATIONWIDGET_H

#ifndef HICARWIDGET_H
#define HICARWIDGET_H

#include <QObject>

class HiCarWidget : public QObject
{
    Q_OBJECT
public:
    explicit HiCarWidget(QObject *parent = nullptr);

signals:

public slots:
};

#endif // HICARWIDGET_H
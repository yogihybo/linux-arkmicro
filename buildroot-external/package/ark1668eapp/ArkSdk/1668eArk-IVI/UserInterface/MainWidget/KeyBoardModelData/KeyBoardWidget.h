#ifndef KEYBOARDWIDGET_H
#define KEYBOARDWIDGET_H

#include <QObject>
class KeyBoardWidgetPrivate;
class KeyBoardWidget : public QObject
{
    Q_OBJECT
public:
    explicit KeyBoardWidget(QObject *parent = nullptr);
    void setKeyBoardWidgetObject(QObject *qmlObject);
    void setKeyBoardLoaderObject(QObject *qmlObject);
public slots:
    void onKeyBoardWidgetVisibel(bool visible);

private:
    KeyBoardWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(KeyBoardWidget)
};

#endif // KEYBOARDWIDGET_H

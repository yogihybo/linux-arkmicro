#ifndef QMLWIDGET_H
#define QMLWIDGET_H

#include <QObject>

class QmlWidget : public QObject
{
    Q_OBJECT
public:
    explicit QmlWidget(QObject *parent = nullptr);
    static QmlWidget* instance();
    Q_INVOKABLE void caseSwitchBtnClicked();//KeyBoardWidget
    Q_INVOKABLE void enterBtnClicked();//KeyBoardWidget
    Q_INVOKABLE void symbolsBtnClicked();//KeyBoardWidget
    Q_INVOKABLE void keyBoardWidgetVisibel(bool visible);
    Q_INVOKABLE void addKeyBoardInputStr(QString str);
    Q_INVOKABLE void subKeyBoardInputStr();
    Q_INVOKABLE void requestMuteToggole();
    Q_INVOKABLE int  getVolumeType();
    void clearKeyBoardInputStr();
signals:
    void onCaseSwitchBtnClicked();//KeyBoardWidget
    void onEnterBtnClicked();//KeyBoardWidget
    void onSymbolsBtnClicked();//KeyBoardWidget
    void onKeyBoardWidgetVisibel(bool visible);//KeyBoardWidget
    void onAddKeyBoardInputStr(QString str);
    void onSubKeyBoardInputStr(QString str);
    void onPhoneLinkMsgShowWidgetShow(QString str);
private:
    QString m_KeyBoardInputStr;
};

#endif // QMLWIDGET_H

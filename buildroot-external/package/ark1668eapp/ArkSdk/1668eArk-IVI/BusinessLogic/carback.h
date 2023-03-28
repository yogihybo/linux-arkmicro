#ifndef CARBACK_H
#define CARBACK_H

#include <QObject>
#include <QGuiApplication>
//struct vin_screen {
//    unsigned int  disp_x;
//    unsigned int  disp_y;
//    unsigned int  disp_width;
//    unsigned int  disp_height;
//};

class CarbackPrivate;
class Carback : public QObject
{
    Q_OBJECT
#ifdef g_Carback
#undef g_Carback
#endif
#define g_Carback (Carback::instance())
public:
    inline static Carback* instance() {
        static Carback *carback(new Carback(qApp));
        return carback;
    }
    void initialize();
    void localDeviceName();
    enum CarbackStatus {
        CBS_Undefine = -1,
        CBS_Off,
        CBS_On,
    };
    enum SignalStatus {
        ACS_Undefine = -1,
        ACS_NoDetectSignal,
        ACS_DetectSignal,
    };
signals:
    void onCarbackStatusChange(int status);
    void onDetectSignal(int status);
private:
    explicit Carback(QObject *parent = nullptr);
    ~Carback();
    CarbackPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(Carback)
};

#endif // CARBACK_H

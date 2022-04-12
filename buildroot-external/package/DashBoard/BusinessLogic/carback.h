#ifndef CARBACK_H
#define CARBACK_H

#include <QObject>
#include <QApplication>

class Carback : public QObject
{
    Q_OBJECT
#define g_Carback (Carback::instance())
public:
    struct vin_screen {
        unsigned int  disp_x;
        unsigned int  disp_y;
        unsigned int  disp_width;
        unsigned int  disp_height;
    };
    enum CarbackStatus {
        CBS_Undefine = -1,
        CBS_Off,
        CBS_On,
    };
    inline static Carback* instance() {
        static Carback *carback(new Carback(qApp));
        return carback;
    }
    void initialize();
signals:
    void CarbackStatusChange(int status);

private:
    explicit Carback(QObject *parent = nullptr);
    ~Carback();
    static void* readCarbckStatus(void *arg);
    int exitThread;
    int carbackFd;
};

#endif // CARBACK_H

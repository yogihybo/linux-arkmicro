#ifndef COMMON_H
#define COMMON_H


#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))
#define MIN(X,Y) (X) < (Y) ? (X) : (Y)
#define MAX(X,Y) (X) > (Y) ? (X) : (Y)

struct Size{
    int width;
    int height;
};

struct Rect{
    int x;
    int y;
    int width;
    int height;
};

struct Point{
    int x;
    int y;
};

enum ScreenPowerMode {
    // see <https://android.googlesource.com/platform/frameworks/base.git/+/pie-release-2/core/java/android/view/SurfaceControl.java#305>
    SPM_OFF = 0,
    SPM_NORMAL = 2,
};

enum MirrorMode {

    MIRROR_OFF = 0,
    MIRROR_ON = 1,
};




class common
{
public:
    common();
};

#endif // COMMON_H

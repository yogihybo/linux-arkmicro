#pragma once

/*!
 * \brief 亿连解码渲染接口
 */
class ECMirrorRender
{
public:
    /*!
     * \brief 镜像方向
     */
    enum MirrorDirection {
        LANDSCAPE, /*! 横屏 */
        VERTICAL,  /*! 竖屏 */
    };

public:
    /*!
     * \brief 初始化解码器
     * 根据实现需要内部可直接使用成员变量中的屏幕和镜像分辨率
     */
    virtual void initialize() = 0;

    /*!
     * \brief 解码渲染
     * \param[in] data 镜像数据
     * \param[in] len 镜像数据长度
     * \param[in] direction 镜像的显示方向，即横屏或竖屏
     */
    virtual void play(const void *data, int len, MirrorDirection direction = LANDSCAPE) = 0;

    /*!
     * \brief 显示镜像画面
     */
    virtual void show() = 0;

    /*!
     * \brief 隐层镜像画面
     */
    virtual void hide() = 0;

    /*!
     * \brief 释放解码器
     */
    virtual void release() = 0;

    /*!
     * \brief 设置屏幕分辨率
     * 该方法会在初始化之前被调用
     */
    inline void setScreenSize(int screenW, int screenH) {
        mScreenW = screenW;
        mScreenH = screenH;
    }

    /*!
     * \brief 设置镜像分辨率
     * 该方法会在初始化之前被调用
     */
    inline void setMirrorSize(int mirrorW, int mirrorH) {
        mMirrorW = mirrorW;
        mMirrorH = mirrorH;
    }

protected:
    int mScreenW; /*! 屏幕宽度 */
    int mScreenH; /*! 屏幕高度 */
    int mMirrorW; /*! 镜像宽度 */
    int mMirrorH; /*! 镜像宽度 */
};


#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include <string>
using namespace std;
#ifdef __cplusplus
extern "C" {
#endif
#include "net.h"

#ifdef __cplusplus
};
#endif

class Server
{    

    enum SERVER_START_STEP {
         SSS_NULL,
         SSS_PUSH,
         SSS_ENABLE_TUNNEL_REVERSE,
         SSS_ENABLE_TUNNEL_FORWARD,
         SSS_EXECUTE_SERVER,
         SSS_RUNNING,
     };

public:
    struct ServerParams {
        string serial = "";            // 设备序列号
        unsigned short localPort = 27183;      // reverse时本地监听端口
        unsigned short maxSize = 0;          // 视频分辨率
        unsigned int bitRate = 0;      // 视频比特率
        string crop = "-";             // 视频裁剪
        bool sendFrameMeta = false;     // 是否发送mp4帧数据
        bool control = true;            // 安卓端是否接收键鼠控制
        bool useReverse = true;         // true:先使用adb reverse，失败后自动使用adb forward；false:直接使用adb forward
    };

    explicit Server();

   void SetServerStartResultCallback(void (*callback)(bool ,void*), void* parameter);
   void SetConnectToResultCallback(void (*callback)(bool, string, Size, void*), void* parameter);

   bool start(Server::ServerParams params);
   bool server_connect_to() ;

   bool isReverse();
   Server::ServerParams getParams();
   socket_t getVideoSocket();
   socket_t getControlSocket();

   void stop();
protected:

private:
    const   string& getServerPath();
    bool    pushServer();
    bool    enableTunnelReverse();
    bool    disableTunnelReverse();
    bool    enableTunnelForward();
    bool    disableTunnelForward();
    bool    execute();
    bool    startServerByStep();
    bool    readInfo(socket_t videoSocket, string& deviceName, Size& size);


private:
    string             m_serverPath = "";
public:
    socket_t m_serverSocket = INVALID_SOCKET;
    socket_t m_videoSocket = INVALID_SOCKET;
    socket_t m_controlSocket = INVALID_SOCKET;
private:


    bool                m_tunnelEnabled = false;
    bool                m_tunnelForward = false; // use "adb forward" instead of "adb reverse"
    unsigned int m_acceptTimeoutTimer = 0;
    unsigned int m_connectTimeoutTimer = 0;
    unsigned int m_connectCount = 0;
    unsigned int m_restartCount = 0;
    string m_deviceName = "";
    Size m_deviceSize = {0, 0};
    ServerParams        m_params;
    SERVER_START_STEP   m_serverStartStep = SSS_NULL;


    void (*m_server_start_result_callback)(bool ,void*);
    void (*m_connect_to_result_callback)(bool, string, Size, void*);
    void *m_parameter;

};

#endif // SERVER_H

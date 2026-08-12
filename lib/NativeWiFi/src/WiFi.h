#ifndef WiFi_h
#define WiFi_h

#include "IPAddress.h"

class WiFiClass {
public:
    int32_t RSSI();
    IPAddress localIP();
    IPAddress subnetMask();
    IPAddress gatewayIP();
    // 模拟器: 固定模拟已连接 (真机用真实 WiFi 状态)
    bool isConnected();
};

extern WiFiClass WiFi;

#endif

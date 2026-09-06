#ifndef __CONFIG_H__
#define __CONFIG_H__

// board_config.h 为作者使用的板子的引脚定义, 不包含配置, 无需修改
#include "board_config.h"

// 型号, 用于检查更新, 如果你使用其他开发板请修改型号, 避免被小程序推送更新
#ifndef MODEL
#define MODEL ""
#endif
// 版本号
#define VERSION "V0.0.2"
// 版本代码, 用于检查更新
#define VERSION_CODE 2

/****************************** 硬件配置 ******************************/
// 可选值参考 https://github.com/ZinggJM/GxEPD2/blob/master/examples/GxEPD2_Example/GxEPD2_display_selection_new_style.h
// 墨水屏种类
#ifndef EPD_TYPE
#define EPD_TYPE GxEPD2_3C
#endif
// 墨水屏驱动
#ifndef EPD_DRIVER
#define EPD_DRIVER GxEPD2_213c
#endif

#ifndef HAS_CONFIG
// 墨水屏引脚
#define EPD_CS -1
#define EPD_DC -1
#define EPD_RST -1
#define EPD_BUSY -1
#if !defined(ESP8266) // ESP8266 不可设置 SPI 引脚
#define EPD_CLK -1
#define EPD_MOSI -1
#endif
// 墨水屏旋转方向, 0~3
#define EPD_ROTATION 0
// 屏幕是否支持局部刷新 (若休眠时屏幕只显示标题栏说明屏幕或驱动不支持局部刷新, 请将宏设为 false)
#define SUPPORT_PARTIAL_UPDATE EPD_DRIVER::hasPartialUpdate
// 切换界面按键引脚
#define KEY_SWITCH -1
// 按键引脚上下拉模式, INPUT/INPUT_PULLUP/INPUT_PULLDOWN
#define KEY_PIN_MODE INPUT
// 按键引脚触发方式, LOW/HIGH
#define KEY_TRIGGER_LEVEL LOW
// 硬件是否支持深睡 (例如 ESP8266 需要将 gpio16 与 rst 相连才可从深睡中唤醒)
#define SUPPORT_DEEP_SLEEP true
#endif

// 电池/外接电源检测 (参考 LiClock, 供自绘驱动板启用; 默认 -1 = 无检测)
#ifndef PIN_BATTERY_ADC
#define PIN_BATTERY_ADC -1    // 电池电压分压 ADC 引脚
#endif
#ifndef PIN_CHARGING
#define PIN_CHARGING -1       // 充电状态引脚 (低电平=充电中)
#endif
// 无电池检测时是否视为 USB 外接供电 (状态栏显示 USB 图标而非假电池)
#ifndef POWER_SOURCE_USB
#define POWER_SOURCE_USB false
#endif
// 电池分压满量程对应电压(mV), 由硬件分压电阻决定 (LiClock 为 7230)
#ifndef BATTERY_ADC_FULL_MV
#define BATTERY_ADC_FULL_MV 7230
#endif

#if (EPD_CS == -1) || (EPD_DC == -1) || (EPD_RST == -1) || (EPD_BUSY == -1) || (!defined(ESP8266) && (EPD_CLK == -1 || EPD_MOSI == -1)) || (KEY_SWITCH == -1)
#warning "请先在 config.h 中配置硬件引脚"
#error "Please configure hardware pins first in config.h"
#endif

/****************************** 软件配置 ******************************/
// 根据屏幕大小选择布局, 可取值如下:
// -1 // 根据屏幕分辨率自动计算最合适的布局
// UISize::XS // 1.54 寸, 分辨率 <=200x200
// UISize::SM // 2.13/2.66/2.9 寸
// UISize::MD // 3.7 寸
// UISize::LG // 4.2 寸
// UISize::XL // 5.8/7.5 寸, 分辨率 >= 640x480
// UISize::CUSTOM // 自定义布局, 在 ui_custom.cpp 中自行适配屏幕
#define UI_RESOLUTION UISize::CUSTOM
// 是否使用完整字库 (推荐启用, 否则连“霾”字都没有)
#define USE_FULL_GB2313_FONT true
// 是否启用开屏加载界面
#define ENABLE_LOADING_SCREEN true
// 是否启用电量显示
#define ENABLE_BATTERY_DISPLAY false
// 电量低于多少百分比时进入休眠模式
#define BATTERY_LOW_PERCENTAGE 5
// 指定秒数无操作后进入休眠模式, 0 为不休眠 (板级可覆盖)
#if SUPPORT_DEEP_SLEEP
#ifndef SLEEP_TIMEOUT
#define SLEEP_TIMEOUT 300 // 默认 5 分钟无操作进入深睡 (验证用)
#endif
#else
#define SLEEP_TIMEOUT 0
#endif
// 夜晚暂停更新或光线较暗时暂停更新
#define SLEEP_ON_NIGHT false
// 时区, 可取值请参考 TZ.h 或 https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#if defined(ESP8266)
#include <TZ.h>
#define TIMEZONE TZ_Asia_Shanghai
#elif defined(ESP32)
#define TIMEZONE "CST-8"
#endif
// NTP服务器地址, 最多可填写三个, 用逗号隔开, 服务器列表请见 https://dns.icoa.cn/ntp/
#define NTP_SERVERS "time.pool.aliyun.com", "pool.ntp.org"
// 配网二维码内容, 可以换成你自己的链接或小程序
#define WIFI_CONFIG_URL "http://wx.ai-thinker.com/api/old/wifi/config"
// 如果配网失败可以取消注释并填入 WiFi 和密码 (注意: 除 ESP32-C5 外均只支持 2.4g WiFi)
// #define WIFI_SSID "SSID"
// #define WIFI_PASSWORD "PASSWORD"
// 和风天气 API Host
#define QWEATHER_HOST "nh5rk8adqv.re.qweatherapi.com"
// 和风天气 API KEY
#define QWEATHER_KEY "f1d328d210914729b48a9455f64d9516"
// 是否显示按小时天气预报温度折线 (仅当屏幕大小足够显示时生效, 例如 3.5 寸以上)
#define SHOW_WEATHER_HOURLY_CURVE false
// 是否显示按天天气预报温度折线 (仅当屏幕大小足够显示时生效, 例如 3.5 寸以上)
#define SHOW_WEATHER_DAILY_CURVE true
// 首页应用区选中高亮的灰度 (0~100): 0=纯白, 100=纯黑, 中间值为灰阶
#define HIGHLIGHT_GRAY 60

// 恭喜你, 已经完成了所有配置, 其余配置可通过小程序或 HTTP 接口修改, 详见 README.md

#endif // __CONFIG_H__

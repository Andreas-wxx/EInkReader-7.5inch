#ifndef __DRAW_H__
#define __DRAW_H__

#include "config.h"

// 启用或禁用GxEPD2_GFX基类, 可用于将引用或指针作为参数传递到显示实例, 额外占用~1.2k代码
#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_7C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <qrcode.h>

#include "GxEPD2_Extra.h"

typedef EPD_TYPE<EPD_DRIVER, EPD_DRIVER::HEIGHT> EPD_CLASS;

void startDraw(EPD_CLASS &epd, int32_t bgcolor = GxEPD_WHITE);
void endDraw(EPD_CLASS &epd, bool partial_update = false);
void drawCenteredString(U8G2_FOR_ADAFRUIT_GFX &u8g2, uint16_t x, uint16_t y, const char *str);
// 应用区 (底部应用条) 独立绘制: 不清屏, 供模拟器方向键局部刷新调用
void drawAppBar(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2);
void drawHomeHint(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2);
// 灰度抖动填充 (0~100): 在 1bit 屏幕上用 Bayer 抖动模拟灰阶
void fillGrayRect(EPD_CLASS &epd, int x, int y, int w, int h, uint8_t gray);
void drawTitleBar(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2, const char *title, bool sleeping, int8_t rssi, int8_t battery);
void drawArrow(EPD_CLASS &epd, uint16_t x, uint16_t y, int16_t asize, float aangle, uint16_t pwidth, uint16_t plength);
void drawQRCode(EPD_CLASS &epd, uint16_t x, uint16_t y, uint8_t scale, const char *text, uint8_t version = 3, uint8_t ecc = ECC_LOW);
// 首页分钟级局部刷新时间 (不清全屏, 用于墨水屏时钟走动)
void refreshHomeClock(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2);
// 休眠(锁屏)界面分钟级局部刷新时间 (横屏, 供 light-sleep 周期唤醒调用)
void refreshLockClock(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2);

const char *getWeatherIcon(uint16_t id, bool fill = false);

#endif // __DRAW_H__

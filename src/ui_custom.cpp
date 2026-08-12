#include "ui.h"

#include "main.h"
#include "lang.h"
#include "bitmap.h"
#include "util.h"

template <>
void UIImpl<UISize::CUSTOM>::loading(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
}

template <>
void UIImpl<UISize::CUSTOM>::smartConfig(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
}

template <>
void UIImpl<UISize::CUSTOM>::syncTimeFailed(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
}

template <>
void UIImpl<UISize::CUSTOM>::lowPower(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
}

template <>
void UIImpl<UISize::CUSTOM>::update(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
}

template <>
void UIImpl<UISize::CUSTOM>::titleBar(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2, tm *ptime, bool sleeping, int8_t rssi, int8_t battery) {
}

template <>
void UIImpl<UISize::CUSTOM>::weather(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2, tm *ptime, bool sleeping, int8_t rssi, int8_t battery) {
}

template <>
void UIImpl<UISize::CUSTOM>::bilibili(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
}

template <>
void UIImpl<UISize::CUSTOM>::display(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2, const String &ip, const String &text) {
}

template <>
void UIImpl<UISize::CUSTOM>::about(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2, const String &ip) {
}

template <>
void UIImpl<UISize::CUSTOM>::bookshelf(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
    startDraw(epd);
    uint16_t w = epd.width();
    uint16_t h = epd.height();

    // title bar
    u8g2.setFont(u8g2_font_wqy14_t);
    u8g2.drawUTF8(10, 24, "我的书架");
    epd.drawFastHLine(0, 32, w, GxEPD_BLACK);

    // book placeholders
    const char *books[] = {"三体", "活着", "小王子"};
    for (int i = 0; i < 3; i++) {
        int y = 50 + i * 80;
        epd.drawRect(10, y, w - 20, 60, GxEPD_BLACK);
        u8g2.setFont(u8g2_font_wqy12_t);
        u8g2.drawUTF8(20, y + 25, books[i]);
        epd.drawRect(20, y + 40, w - 60, 8, GxEPD_BLACK);
        epd.fillRect(20, y + 40, (w - 60) / 3, 8, GxEPD_BLACK);
    }

    // bottom hint
    u8g2.setFont(u8g2_font_wqy12_t);
    u8g2.drawUTF8(10, h - 20, "F2 切换页面");
    endDraw(epd);
}

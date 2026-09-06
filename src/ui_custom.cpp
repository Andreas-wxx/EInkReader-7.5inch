#include "ui.h"

#include "main.h"
#include <WiFi.h>
#include "lang.h"
#include "bitmap.h"
#include "util.h"
#include "lunar.h"
#include "romannum_bitmap.h"

// 右侧隐藏式应用栏宽度 (480 宽竖屏中占 170px)
#define APP_BAR_W 170

// ============================================================
// 顶部状态栏 & 锁屏(低功耗)界面
// 电池/WiFi 图标来源: LiClock (battery.cpp / AppManager.cpp), xbm 格式
// ============================================================

// 电池图标 (20x16, 每行3字节存储)
static const uint8_t battery_empty_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 0x80, 0x00, 0x80,
    0x80, 0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0xe0, 0x80, 0x00, 0xe0,
    0x80, 0x00, 0xe0, 0x80, 0x00, 0xe0, 0x80, 0x00, 0x80, 0x80, 0x00, 0x80,
    0x80, 0x00, 0x80, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t battery_quarter_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 0x80, 0x07, 0x80,
    0x80, 0x07, 0x80, 0x80, 0x07, 0x80, 0x80, 0x07, 0xe0, 0x80, 0x07, 0xe0,
    0x80, 0x07, 0xe0, 0x80, 0x07, 0xe0, 0x80, 0x07, 0x80, 0x80, 0x07, 0x80,
    0x80, 0x07, 0x80, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t battery_three_quarters_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 0x87, 0xff, 0x80,
    0x87, 0xff, 0x80, 0x87, 0xff, 0x80, 0x87, 0xff, 0xe0, 0x87, 0xff, 0xe0,
    0x87, 0xff, 0xe0, 0x87, 0xff, 0xe0, 0x87, 0xff, 0x80, 0x87, 0xff, 0x80,
    0x87, 0xff, 0x80, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t battery_full_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 0xff, 0xff, 0x80,
    0xff, 0xff, 0x80, 0xff, 0xff, 0x80, 0xff, 0xff, 0xe0, 0xff, 0xff, 0xe0,
    0xff, 0xff, 0xe0, 0xff, 0xff, 0xe0, 0xff, 0xff, 0x80, 0xff, 0xff, 0x80,
    0xff, 0xff, 0x80, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t flash_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x0E, 0x00,
    0x00, 0x1F, 0x00, 0x80, 0x1F, 0x00, 0xC0, 0xFF, 0x3F, 0xE0, 0xFF, 0x0F,
    0xF0, 0xFF, 0x07, 0xF8, 0xFF, 0x01, 0x00, 0xFE, 0x00, 0x00, 0x3F, 0x00,
    0x00, 0x1F, 0x00, 0x00, 0x07, 0x00, 0x80, 0x03, 0x00, 0x80, 0x00, 0x00};

static const uint8_t battery_half_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 0x80, 0x7f, 0x80,
    0x80, 0x7f, 0x80, 0x80, 0x7f, 0x80, 0x80, 0x7f, 0xe0, 0x80, 0x7f, 0xe0,
    0x80, 0x7f, 0xe0, 0x80, 0x7f, 0xe0, 0x80, 0x7f, 0x80, 0x80, 0x7f, 0x80,
    0x80, 0x7f, 0x80, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
// (充电显示改用 flash_bits 闪电标识 + 100%, 见 drawStatusBar)
// (状态栏电池充电态改用 battery_charging_bits; liclock 的 usb 图标已弃用)
// WiFi 图标 (16x13)
static const uint8_t wifiIcon[] = {
    0x00, 0x00, 0xf0, 0x0f, 0xfc, 0x3f, 0x1e, 0x78, 0x07, 0xe0, 0xe0, 0x07,
    0xf8, 0x1f, 0x38, 0x1c, 0x00, 0x00, 0x80, 0x01, 0xc0, 0x03, 0x80, 0x01,
    0x00, 0x00};

// 顶部状态栏: 右上角 WiFi 图标 + 电池/充电图标 (高 26px)
static void drawStatusBar(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
    uint16_t w = epd.width();
    // 左侧: 当前时间
    time_t timestamp = time(nullptr);
    tm *ptime = localtime(&timestamp);
    char timeBuf[8];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", ptime->tm_hour, ptime->tm_min);
    u8g2.setFont(u8g2_font_helvB14_tf);
    u8g2.drawUTF8(8, 18, timeBuf);

    // 右侧 (从右往左依次画): 电量百分比 + 电池图标 + WiFi
    // 视觉从左到右: WiFi -> 电池(0/25/50/75/100%, 充电=满格白闪) -> 百分比
    int16_t x = w - 2;
    int8_t batt = getBatteryLevel();      // -1 = 无电池检测
    bool charging = isCharging();         // 充电中 / USB 外接供电
    if (batt < 0) batt = 0;               // 无电池接入: 按 0% 显示
    // 电量百分比 (1% 精度): 充电/USB供电显示 100% (电池充满但未拔USB的供电态)
    {
        char pctBuf[8];
        snprintf(pctBuf, sizeof(pctBuf), "%d%%", charging ? 100 : batt);
        u8g2.setFont(u8g2_font_helvB14_tf);
        uint16_t tw = u8g2.getUTF8Width(pctBuf);
        x -= tw + 2;
        u8g2.drawUTF8(x, 18, pctBuf);
        x -= 6;
    }
    // 电池图标 (20x16): 充电=闪电标识; 否则按 5 档电量
    {
        const uint8_t *icon;
        if (charging) {
            icon = flash_bits;
        } else if (batt < 13)      icon = battery_empty_bits;         // 0%
        else if (batt < 38)        icon = battery_quarter_bits;       // 25%
        else if (batt < 63)        icon = battery_half_bits;          // 50%
        else if (batt < 88)        icon = battery_three_quarters_bits;// 75%
        else                       icon = battery_full_bits;          // 100%
        x -= 20;
        epd.drawXBitmap(x, 5, icon, 20, 16, GxEPD_BLACK);
        x -= 4;
    }
    // WiFi 图标 (16x13): 有网=扇区; 无网=黑圆白叉(不可用)
    x -= 16;
    if (WiFi.isConnected()) {
        epd.drawXBitmap(x, 7, wifiIcon, 16, 13, GxEPD_BLACK);
    } else {
        int16_t cx = x + 8, cy = 13;
        epd.fillCircle(cx, cy, 6, GxEPD_BLACK);                 // 黑圆
        epd.drawLine(cx - 3, cy - 3, cx + 3, cy + 3, GxEPD_WHITE); // 白叉
        epd.drawLine(cx - 3, cy + 3, cx + 3, cy - 3, GxEPD_WHITE);
    }
    // 不画分割线: 状态栏与页面内容共享顶部空间
}

// 首页顶部时间块: 大时间(92px) + 公历日期 + 农历/节气/节假日
// 供首页整页绘制与分钟级局部刷新共用
static void drawHomeClockBody(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2, tm *ptime) {
    uint16_t w = epd.width();
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", ptime->tm_hour, ptime->tm_min);
    u8g2.setFont(u8g2_font_logisoso92_tn);
    int tw = u8g2.getUTF8Width(buf);
    u8g2.drawStr((w - tw) / 2, 170, buf);

    // 公历日期
    static const char *week[] = {"日", "一", "二", "三", "四", "五", "六"};
    char date[32];
    snprintf(date, sizeof(date), "%d月%d日 周%s", ptime->tm_mon + 1, ptime->tm_mday, week[ptime->tm_wday]);
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    tw = u8g2.getUTF8Width(date);
    u8g2.drawUTF8((w - tw) / 2, 225, date);

    // 农历 + 传统节气 + 法定/传统节假日
    char lunarBuf[96];
    LunarDate ld;
    if (solarToLunar(ptime->tm_year + 1900, ptime->tm_mon + 1, ptime->tm_mday, ld)) {
        snprintf(lunarBuf, sizeof(lunarBuf), "农历%s%s月%s", ld.isLeap ? "闰" : "", lunarMonthCn(ld.month), lunarDayCn(ld.day));
        // 节假日优先 (如清明 -> 清明节); 没有节假日才显示节气 (如大暑/小暑)
        if (ld.festival) {
            snprintf(lunarBuf + strlen(lunarBuf), sizeof(lunarBuf) - strlen(lunarBuf), " · %s", ld.festival);
        } else if (ld.term) {
            snprintf(lunarBuf + strlen(lunarBuf), sizeof(lunarBuf) - strlen(lunarBuf), " · %s", termName(ld.term));
        }
    } else {
        snprintf(lunarBuf, sizeof(lunarBuf), "");
    }
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    tw = u8g2.getUTF8Width(lunarBuf);
    u8g2.drawUTF8((w - tw) / 2, 265, lunarBuf);
}

// 首页分钟级局部刷新: 只刷顶部状态栏条 + 时间块两个窗口, 不清全屏不闪烁
// 前提: 首页已由 UI::home 整页显示过 (buffer 内容保留在 RAM)
void refreshHomeClock(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
    uint16_t w = epd.width();
    time_t timestamp = time(nullptr);
    tm *ptime = localtime(&timestamp);
    // 注意: 不要 init()! init 会把 _initial_refresh 置 true, 导致区域刷新被
    // 强制转成全屏全刷(黑一下)。首页整页显示后控制器 RAM 保持(见 draw.cpp endDraw),
    // 直接对 buffer 改局部内容 + displayWindow 即可做真局部刷新。
    // 窗口1: 顶部状态栏条 (含左侧时间/右侧 WiFi+电源), y 0..40
    epd.fillRect(0, 0, w, 40, GxEPD_WHITE);
    drawStatusBar(epd, u8g2);
    epd.displayWindow(0, 0, w, 40);
    // 窗口2: 大时间 + 日期 + 农历 (y 72..296, 避开中部天气区分割线 y300)
    epd.fillRect(0, 72, w, 224, GxEPD_WHITE);
    drawHomeClockBody(epd, u8g2, ptime);
    epd.displayWindow(0, 72, w, 224);
}

// 锁屏大时间: 位图数字横向绘制, 整体居中, 字符间留间距 (用于横屏 800x480)
static void drawLockTime(EPD_CLASS &epd, int16_t topY, const char *time) {
    uint16_t w = epd.width();
    const int16_t GAP = 20; // 数字间距, 避免挤在一起
    int total = 0;
    for (const char *p = time; *p; p++) {
        uint16_t gw, gh;
        if (rnum_glyph((uint8_t) *p, gw, gh)) total += gw + GAP;
    }
    total -= GAP; // 最后一个字符后不加
    int x = (w - total) / 2;
    for (const char *p = time; *p; p++) {
        uint16_t gw, gh;
        const uint8_t *bits = rnum_glyph((uint8_t) *p, gw, gh);
        if (bits) {
            // 冒号位图比数字矮, 垂直居中下移, 否则冒号两点偏上
            int16_t dy = (*p == ':') ? (RNUM_0_H - RNUM_COLON_H) / 2 : 0;
            epd.drawXBitmap(x, topY + dy, bits, gw, gh, GxEPD_BLACK);
            x += gw + GAP;
        }
    }
}

// 锁屏界面日期行: "x月x日 周x + 农历 + 节气/节假日", 居中于 y=288 (横屏)
// lowPower(整页) 与 refreshLockClock(跨天局部刷新) 共用
static void drawLockDateLine(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2, tm *ptime) {
    uint16_t w = epd.width();
    static const char *week[] = {"日", "一", "二", "三", "四", "五", "六"};
    char dateBuf[96];
    snprintf(dateBuf, sizeof(dateBuf), "%d月%d日 周%s", ptime->tm_mon + 1, ptime->tm_mday, week[ptime->tm_wday]);
    LunarDate ld;
    if (solarToLunar(ptime->tm_year + 1900, ptime->tm_mon + 1, ptime->tm_mday, ld)) {
        char lunarBuf[72];
        snprintf(lunarBuf, sizeof(lunarBuf), "%s农历%s%s月%s", "   ", ld.isLeap ? "闰" : "", lunarMonthCn(ld.month), lunarDayCn(ld.day));
        if (ld.festival) {
            snprintf(lunarBuf + strlen(lunarBuf), sizeof(lunarBuf) - strlen(lunarBuf), "%s%s", "   ", ld.festival);
        } else if (ld.term) {
            snprintf(lunarBuf + strlen(lunarBuf), sizeof(lunarBuf) - strlen(lunarBuf), "%s%s", "\u3000", termName(ld.term));
        }
        strncat(dateBuf, lunarBuf, sizeof(dateBuf) - strlen(dateBuf) - 1);
    }
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    drawCenteredString(u8g2, w / 2, 288, dateBuf);
}

// 休眠期每分钟局部刷新锁屏界面时间 (不清全屏不闪烁)
// 前提: UI::lowPower 已整页显示过且未 hibernate (RAM 保留)
void refreshLockClock(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
    uint16_t w = epd.width(); // 800 (横屏)
    time_t ts = time(nullptr);
    tm *pt = localtime(&ts);
    static int lastYday = -1;
    // 窗口1: 超大时间 (y 56..248, drawLockTime topY=60 高约176)
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", pt->tm_hour, pt->tm_min);
    epd.fillRect(0, 56, w, 192, GxEPD_WHITE);
    drawLockTime(epd, 60, buf);
    epd.displayWindow(0, 56, w, 192);
    // 窗口2: 日期行 (仅跨天时刷新, y 260..292)
    // 注意: 不能延伸到 >=293, 否则会削掉温度曲线最高温数值的"脑袋"
    //       (最高温点 y+12=327, 标注基线 317, simhei24 字顶约 295)
    if (pt->tm_yday != lastYday) {
        lastYday = pt->tm_yday;
        epd.fillRect(0, 260, w, 32, GxEPD_WHITE);
        drawLockDateLine(epd, u8g2, pt);
        epd.displayWindow(0, 260, w, 32);
    }
}


// 应用图标绘制 (Windows 桌面风格), 定义在文件末尾
static void drawAppIcon(EPD_CLASS &epd, int cx, int y, uint8_t type, uint16_t color);
// 首页天气折线绘制辅助函数
static void drawHomeHourlyCurve(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2, uint16_t y, uint16_t h, HourlyForecast &forecast);
static void drawHomeDailyCurve(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2, uint16_t y, uint16_t h, int dayOfWeek, DailyForecast &forecast);

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
    // 锁屏(低功耗)界面: 横屏 800x480
    // 注意: 进入前 lockScreen() 已调用 epd.setRotation(0) 切到横屏
    startDraw(epd);
    uint16_t w = epd.width();   // 800 (横屏)
    uint16_t h = epd.height();  // 480

    time_t timestamp = time(nullptr);
    tm *ptime = localtime(&timestamp);
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", ptime->tm_hour, ptime->tm_min);

    // 超大数字时间 (顶部 y=60, 高176)
    drawLockTime(epd, 60, buf);

    // 日期 + 星期 + 农历/节气/节假日 (与时间拉开间距)
    drawLockDateLine(epd, u8g2, ptime);

    // 按小时温度折线 (锁屏也显示, 条带压缩避免过高), 数据用全局缓存
    HourlyForecast hourlyForecast = {
        .weather = weatherCache.hourly,
        .length = ARRAY_LENGTH(weatherCache.hourly),
        .interval = config.hour_step
    };
    if (weatherCache.valid) {
        drawHomeHourlyCurve(epd, u8g2, 315, 70, hourlyForecast);
    }

    // 室温湿度: 放在时间标注(折线底)和提示句之间的正中
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    drawCenteredString(u8g2, w / 2, 422, "室温 --℃  湿度 --%");

    // 解锁提示
    u8g2.setFont(u8g2_font_wqy12_t);
    drawCenteredString(u8g2, w / 2, 458, "请按任意键2秒唤醒");

    endDraw(epd);
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

    // 顶部状态栏 (与其他页面一致: 左侧时间 + 右侧网络/电池)
    drawStatusBar(epd, u8g2);

    // title bar (下移到状态栏下方避让)
    u8g2.setFont(u8g2_font_wqy14_t);
    u8g2.drawUTF8(10, 50, "我的书架");
    epd.drawFastHLine(0, 58, w, GxEPD_BLACK);

    // book placeholders
    const char *books[] = {"三体", "活着", "小王子"};
    for (int i = 0; i < 3; i++) {
        int y = 72 + i * 80;
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

template <>
void UIImpl<UISize::CUSTOM>::home(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
    startDraw(epd);
    uint16_t w = epd.width();   // 480 (竖屏)
    uint16_t h = epd.height();  // 800

    // 顶部状态栏 (高 26px): 右上角 WiFi + 电池/充电
    drawStatusBar(epd, u8g2);

    time_t timestamp = time(nullptr);
    tm *ptime = localtime(&timestamp);

    // ================================================================
    // 页面占比 (480x800 竖屏, 用注释标出各区域宽度):
    //   顶部 时间区: y 0~300   (37.5%)  大时间 + 公历 + 农历/节气/节假日
    //                                    右上角叠放状态栏(WiFi+电池), 不占布局
    //   中部 天气区: y 300~640 (42.5%)  今日天气(居中) + 三天折线 + 温湿度
    //   下部 应用区: y 640~800 (20%)    应用图标, 名称几乎顶到底部
    // ================================================================

    // ================= 上部 时间区 (0~300, 37.5%) =================
    // 大时间 + 公历 + 农历/节气/节假日 (与分钟局部刷新 drawHomeClockBody 共用)
    drawHomeClockBody(epd, u8g2, ptime);

    epd.drawFastHLine(0, 300, w, GxEPD_BLACK);

    // ================= 中部 天气区 (300~640, 42.5%) =================
    // 天气数据用全局缓存 (开机+定时刷新), 页面切换不重复请求网络
    Weather currentWeather = weatherCache.current;
    HourlyForecast hourlyForecast = {
        .weather = weatherCache.hourly,
        .length = ARRAY_LENGTH(weatherCache.hourly),
        .interval = config.hour_step
    };
    DailyForecast dailyForecast = {
        .weather = weatherCache.daily,
        .length = ARRAY_LENGTH(weatherCache.daily)
    };
    bool success = weatherCache.valid;

    // 今日天气标题 (居中)
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    drawCenteredString(u8g2, w / 2, 335, "今日天气");

    // 当前天气行 (整体居中: 图标 + 文字)
    const char *icon = getWeatherIcon(currentWeather.icon, isNight(currentWeather.time));
    char nowBuf[48];
    if (success) {
        snprintf(nowBuf, sizeof(nowBuf), "%s %d℃", currentWeather.text.c_str(), currentWeather.temp);
    } else {
        snprintf(nowBuf, sizeof(nowBuf), "--℃");
    }
    u8g2.setFont(u8g2_font_qweather_icon_16);
    int iconW = u8g2.getUTF8Width(icon);
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    int textW = u8g2.getUTF8Width(nowBuf);
    int totalW = iconW + 8 + textW;
    int x0 = (w - totalW) / 2;
    u8g2.setFont(u8g2_font_qweather_icon_16);
    u8g2.drawUTF8(x0, 375, icon);
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    u8g2.drawUTF8(x0 + iconW + 8, 375, nowBuf);

    // 温度范围 + 湿度 (居中)
    char rangeBuf[48];
    if (success) {
        snprintf(rangeBuf, sizeof(rangeBuf), "%d ~ %d℃  湿度 %d%%", weatherCache.daily[0].tempMin, weatherCache.daily[0].tempMax, currentWeather.humidity);
    } else {
        snprintf(rangeBuf, sizeof(rangeBuf), "");
    }
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    drawCenteredString(u8g2, w / 2, 410, rangeBuf);



#if SHOW_WEATHER_HOURLY_CURVE == true
    // 小时温度折线
    u8g2.setFont(u8g2_font_wqy12_t);
    u8g2.drawUTF8(12, 445, "未来24小时温度");
    uint16_t hh = 70;
    drawHomeHourlyCurve(epd, u8g2, 455, hh, hourlyForecast);
    epd.drawFastHLine(0, 455 + hh + 8, w, GxEPD_BLACK);
#endif

#if SHOW_WEATHER_DAILY_CURVE == true
    // 三天温度折线 (min/max 双线, 最高温标曲线上方, 最低温标曲线下方)
    uint16_t dh = 150;
    drawHomeDailyCurve(epd, u8g2, 436, dh, ptime->tm_wday, dailyForecast);
    epd.drawFastHLine(0, 436 + dh + 6, w, GxEPD_BLACK);
#else
    // 无日折线时的简易三天表格
    uint16_t dh = 150;
    drawHomeDailyCurve(epd, u8g2, 436, dh, ptime->tm_wday, dailyForecast);
    epd.drawFastHLine(0, 436 + dh + 6, w, GxEPD_BLACK);
#endif

    // 温湿度一行 (居中)
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    drawCenteredString(u8g2, w / 2, 625, "室温：--℃    湿度：--%");
    if (!success) {
        u8g2.setForegroundColor(COLOR_ERROR);
        drawCenteredString(u8g2, w / 2, 622, "天气获取失败");
        u8g2.setForegroundColor(COLOR_PRIMARY);
    }

    epd.drawFastHLine(0, 640, w, GxEPD_BLACK);

    // ================= 下部 提示区 (640~800, 20%) =================
    // 原应用区改为"按确认键打开应用"提示; 应用栏改到右侧隐藏式(按确认键呼出)
    drawHomeHint(epd, u8g2);

    // 右侧隐藏式应用栏: 呼出时覆盖在首页右侧 (状态栏下方到屏幕底部)
    if (appBarOpen) {
        drawAppBar(epd, u8g2);
    }

    endDraw(epd);
}

// 首页底部提示区 (y 640~800): 原应用区位置, 引导按确认键呼出右侧应用栏
void drawHomeHint(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
    uint16_t w = epd.width();
    // 先擦除提示区为白色, 再重绘 (局部刷新用)
    epd.fillRect(0, 641, w, 159, GxEPD_WHITE);
    epd.drawFastHLine(0, 640, w, GxEPD_BLACK);
    // 居中: 书本图标 + 提示文字
    drawAppIcon(epd, w / 2, 660, 0, GxEPD_BLACK);
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    drawCenteredString(u8g2, w / 2, 755, "按确认键 打开应用");
}

// 右侧隐藏式应用栏: 按确认键呼出, 应用从上到下排列, 高度=状态栏下方到屏幕底部
// 供模拟器局部刷新单独调用 (appBarOpen=false 时只擦白, 不留痕迹)
void drawAppBar(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2) {
    uint16_t w = epd.width();   // 480 (竖屏)
    uint16_t h = epd.height();  // 800
    const char *appNames[] = {"电子书", "设置", "纪念日", "每日诗词"};
    int n = 4;
    int barX = w - APP_BAR_W;   // 右侧应用栏 x 起点
    int topY = 30;              // 状态栏(26px)下方开始
    int barH = h - topY;        // 应用栏高度: 状态栏下方到屏幕底部
    // 先擦除应用栏区域为白色 (局部刷新用; 收起时擦白不留痕)
    epd.fillRect(barX, topY, APP_BAR_W, barH, GxEPD_WHITE);
    if (!appBarOpen) return;
    epd.drawFastVLine(barX, topY, barH, GxEPD_BLACK); // 左侧分隔线
    int itemH = barH / n;
    for (int i = 0; i < n; i++) {
        int iy = topY + i * itemH;
        bool sel = (i == selectedApp);
        if (sel) {
            // 高亮背景: 用灰度抖动填充, 灰度值可调 (见 HIGHLIGHT_GRAY)
            fillGrayRect(epd, barX + 6, iy + 10, APP_BAR_W - 12, itemH - 20, HIGHLIGHT_GRAY);
        }
        // 图标 (条目上半居中) + 名称 (条目下半居中)
        drawAppIcon(epd, barX + APP_BAR_W / 2, iy + itemH / 2 - 52, i, sel ? GxEPD_WHITE : GxEPD_BLACK);
        u8g2.setFont(u8g2_font_simhei24_t_gb2312);
        u8g2.setForegroundColor(sel ? GxEPD_WHITE : GxEPD_BLACK);
        drawCenteredString(u8g2, barX + APP_BAR_W / 2, iy + itemH / 2 + 28, appNames[i]);
        u8g2.setForegroundColor(COLOR_PRIMARY);
    }
}

// 灰度填充: 用 4x4 Bayer 有序抖动在 1bit 屏幕上模拟灰阶
// gray: 0~100 (0=白, 100=纯黑), 可用于墨水屏高亮等灰度效果
void fillGrayRect(EPD_CLASS &epd, int x, int y, int w, int h, uint8_t gray) {
    if (w <= 0 || h <= 0) return;
    if (gray >= 100) { epd.fillRect(x, y, w, h, GxEPD_BLACK); return; }
    if (gray <= 0) return; // 白色
    static const uint8_t bayer[4][4] = {
        {0, 8, 2, 10},
        {12, 4, 14, 6},
        {3, 11, 1, 9},
        {15, 7, 13, 5}
    };
    uint8_t th = (uint8_t)(gray * 16 / 100); // 阈值
    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            if (bayer[yy & 3][xx & 3] < th) {
                epd.drawPixel(x + xx, y + yy, GxEPD_BLACK);
            }
        }
    }
}

// 小时温度折线 (参考 ui_lg.cpp drawForecastHourly)
static void drawHomeHourlyCurve(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2, uint16_t y, uint16_t h, HourlyForecast &forecast) {
    if (forecast.length < 2) return;
    uint16_t grid_w = epd.width() / forecast.length;
    int8_t min_temp = forecast.weather[0].temp;
    int8_t max_temp = forecast.weather[0].temp;
    for (uint8_t i = 0; i < forecast.length; i++) {
        Weather &weather = forecast.weather[i];
        if (weather.temp < min_temp) min_temp = weather.temp;
        if (weather.temp > max_temp) max_temp = weather.temp;
    }
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    uint16_t prev_x = 0;
    uint16_t prev_y = 0;
    for (uint8_t i = 0; i < forecast.length; i++) {
        Weather &weather = forecast.weather[i];
        uint16_t point_x = grid_w * i + grid_w / 2;
        // 曲线底部抬高到 y+h-24, 给最低温的温度数值留空间, 避免与时间标注重叠
        uint16_t point_y = (uint16_t) map(weather.temp, min_temp, max_temp, y + h - 24, y + 12);
        epd.fillCircle(point_x, point_y, 3, COLOR_PRIMARY); // 实心节点
        if (i > 0) {
            epd.drawLine(prev_x, prev_y, point_x, point_y, COLOR_PRIMARY);
        }
        prev_x = point_x;
        prev_y = point_y;
        // 温度数值: 标在曲线点上方 (仿首页日曲线)
        char tbuf[8];
        snprintf(tbuf, sizeof(tbuf), "%d℃", weather.temp);
        drawCenteredString(u8g2, point_x, point_y - 10, tbuf);
        // 时间标注: 比温度数值小一号的常规数字 (helvR14, 非粗体), 放在曲线底部
        u8g2.setFont(u8g2_font_helvR14_tf);
        drawCenteredString(u8g2, point_x, y + h - 2, weather.time.substring(11, 16).c_str());
        u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    }
}

// 三天预报 + 日温度折线 (min/max 双线)
// 最高温标注在最高温曲线点的上方, 最低温标注在最低温曲线点的下方, 避免挤在一起
static void drawHomeDailyCurve(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2, uint16_t y, uint16_t h, int dayOfWeek, DailyForecast &forecast) {
    if (forecast.length < 2) return;
    uint16_t grid_w = epd.width() / forecast.length;
    // 最高温线/最低温线各用独立范围映射, 使两条曲线在窄带内都充分展开, 避免中间大片空白
    int8_t min_hi = forecast.weather[0].tempMax, max_hi = forecast.weather[0].tempMax; // 最高温范围
    int8_t min_lo = forecast.weather[0].tempMin, max_lo = forecast.weather[0].tempMin; // 最低温范围
    for (uint8_t i = 0; i < forecast.length; i++) {
        DailyWeather &weather = forecast.weather[i];
        if (weather.tempMax < min_hi) min_hi = weather.tempMax;
        if (weather.tempMax > max_hi) max_hi = weather.tempMax;
        if (weather.tempMin < min_lo) min_lo = weather.tempMin;
        if (weather.tempMin > max_lo) max_lo = weather.tempMin;
    }
    // 防止范围相同导致 map 除零: 单值时映射到窄带中间
    if (min_hi == max_hi) { min_hi--; max_hi++; }
    if (min_lo == max_lo) { min_lo--; max_lo++; }
    // 垂直居中: 内容块(星期标签顶 ~ 最低温标注底)中心对齐区域中心(y-11 ~ y+h+6 近似 y+h/2)
    // 上/下分割线在 home() 中为 y-11 和 y+h+6, 间距不变, 只把内容整体平移
    int16_t content_top = (int16_t) y + 16 - 20;              // 星期标签顶部 (baseline - 字高)
    int16_t content_bot = (int16_t) y + (int16_t) h * 2 / 3 + 22 + 4; // 最低温标注底部
    int16_t voff = ((int16_t) y + (int16_t) h / 2) - (content_top + content_bot) / 2;
    if (voff < 0) voff = 0;

    // 星期标签
    u8g2.setFont(u8g2_font_simhei24_t_gb2312);
    for (uint8_t i = 0; i < forecast.length; i++) {
        uint16_t x = grid_w * i + grid_w / 2;
        const char *dayText = nullptr;
        if (i == 0) {
            dayText = "今天";
        } else if (i == 1) {
            dayText = "明天";
        } else {
            dayText = WEEKDAYS[(dayOfWeek + i) % 7];
        }
        drawCenteredString(u8g2, x, y + 16 + voff, dayText);
    }
#if SHOW_WEATHER_DAILY_CURVE == true
    // 双条带: 最高温曲线在上条带展开, 最低温曲线在下条带展开, 各用独立范围映射
    // 两条曲线永不纠缠 (最高温总在上, 最低温总在下), 且各自条带内都有起伏
    uint16_t band_top_hi = y + h / 3 + voff;        // 最高温条带上缘
    uint16_t band_bot_hi = y + h / 2 + voff - 8;    // 最高温条带下缘
    uint16_t band_top_lo = y + h / 2 + voff + 8;    // 最低温条带上缘
    uint16_t band_bot_lo = y + h * 2 / 3 + voff;    // 最低温条带下缘
    uint16_t prev_x = 0;
    uint16_t prev_y1 = 0;
    uint16_t prev_y2 = 0;
    for (uint8_t i = 0; i < forecast.length; i++) {
        DailyWeather &weather = forecast.weather[i];
        uint16_t point_x = grid_w * i + grid_w / 2;
        uint16_t point_y1 = (uint16_t) map(weather.tempMin, min_lo, max_lo, band_bot_lo, band_top_lo);
        uint16_t point_y2 = (uint16_t) map(weather.tempMax, min_hi, max_hi, band_bot_hi, band_top_hi);
        epd.fillCircle(point_x, point_y1, 2, COLOR_PRIMARY); // 实心节点
        epd.fillCircle(point_x, point_y2, 2, COLOR_PRIMARY);
        if (i > 0) {
            epd.drawLine(prev_x, prev_y1, point_x, point_y1, COLOR_PRIMARY);
            epd.drawLine(prev_x, prev_y2, point_x, point_y2, COLOR_PRIMARY);
        }
        prev_x = point_x;
        prev_y1 = point_y1;
        prev_y2 = point_y2;
        // 温度标注: 最高温在曲线上方, 最低温在曲线下方
        u8g2.setFont(u8g2_font_simhei24_t_gb2312);
        String tMax = String(weather.tempMax) + "℃";
        drawCenteredString(u8g2, point_x, point_y2 - 8, tMax.c_str());
        String tMin = String(weather.tempMin) + "℃";
        drawCenteredString(u8g2, point_x, point_y1 + 30, tMin.c_str()); // 下移留出与曲线距离
    }
#endif
}

// 简单的应用图标 (Windows 桌面风格: 图标 + 底部名称)
static void drawAppIcon(EPD_CLASS &epd, int cx, int y, uint8_t type, uint16_t color) {
    switch (type) {
    case 0: // 电子书: 书本
        epd.drawRect(cx - 16, y, 32, 42, color);
        epd.drawFastVLine(cx, y, 42, color);
        epd.drawFastHLine(cx - 10, y + 12, 20, color);
        epd.drawFastHLine(cx - 10, y + 22, 20, color);
        epd.drawFastHLine(cx - 10, y + 32, 20, color);
        break;
    case 1: // 设置: 齿轮
        epd.drawCircle(cx, y + 21, 15, color);
        epd.drawCircle(cx, y + 21, 5, color);
        epd.drawFastVLine(cx, y + 1, 8, color);
        epd.drawFastVLine(cx, y + 33, 8, color);
        epd.drawFastHLine(cx - 20, y + 21, 8, color);
        epd.drawFastHLine(cx + 12, y + 21, 8, color);
        break;
    case 2: // 纪念日: 日历
        epd.drawRect(cx - 16, y + 6, 32, 36, color);
        epd.drawFastHLine(cx - 16, y + 16, 32, color);
        epd.drawFastVLine(cx - 11, y, 7, color);
        epd.drawFastVLine(cx + 6, y, 7, color);
        epd.fillRect(cx - 10, y + 24, 6, 6, color);
        epd.fillRect(cx + 2, y + 24, 6, 6, color);
        epd.fillRect(cx - 10, y + 33, 6, 6, color);
        break;
    case 3: // 每日诗词: 纸页 + 文字行
        epd.drawRect(cx - 14, y, 28, 42, color);
        epd.drawFastHLine(cx - 8, y + 12, 16, color);
        epd.drawFastHLine(cx - 8, y + 22, 16, color);
        epd.drawFastHLine(cx - 8, y + 32, 10, color);
        break;
    }
}

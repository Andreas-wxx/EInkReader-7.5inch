// SMPL_2021.7.22_V2 开发板
#ifdef BOARD_SMPL_V2
#define HAS_CONFIG
#define EPD_CS 15
#define EPD_DC 0
#define EPD_RST 2
#define EPD_BUSY 4
#define EPD_ROTATION 3
#define SUPPORT_PARTIAL_UPDATE false
#define KEY_SWITCH 5
#define KEY_PIN_MODE INPUT_PULLUP
#define KEY_TRIGGER_LEVEL LOW
#define SUPPORT_DEEP_SLEEP false
#endif

// 由 OurEDA 设计的 CoreBoard_ESP32 开发板
#ifdef BOARD_COREBOARD_V1
#define HAS_CONFIG
#define EPD_CS 5
#define EPD_DC 6
#define EPD_RST 7
#define EPD_BUSY 8
#define EPD_CLK 4
#define EPD_MOSI 10
#define EPD_ROTATION 3
#define SUPPORT_PARTIAL_UPDATE false
#define KEY_SWITCH 0
#define KEY_PIN_MODE INPUT_PULLUP
#define KEY_TRIGGER_LEVEL LOW
#define SUPPORT_DEEP_SLEEP true
#endif

// 基于 SDL 的模拟器
#ifdef NATIVE
#define HAS_CONFIG
#define EPD_CS 0
#define EPD_DC 0
#define EPD_RST 0
#define EPD_BUSY 0
#define EPD_CLK 0
#define EPD_MOSI 0
#ifndef EPD_ROTATION
#define EPD_ROTATION 0
#endif
#define SUPPORT_PARTIAL_UPDATE false
#define KEY_SWITCH 0
#define KEY_PIN_MODE INPUT
#define KEY_TRIGGER_LEVEL HIGH
#define SUPPORT_DEEP_SLEEP false
#endif

// 微雪 e-Paper ESP32 Driver Board Rev3 (ESP32-WROOM-32E) + 7.5 V2 墨水屏 (GDEY075T7/UC8179)
// 屏幕排线座固定接线: CLK=13 DIN=14 CS=15 RST=26 DC=27 BUSY=25
#ifdef BOARD_WAVESHARE_DRIVER_REV3
#define HAS_CONFIG
#define EPD_CS 15
#define EPD_DC 27
#define EPD_RST 26
#define EPD_BUSY 25
#define EPD_CLK 13
#define EPD_MOSI 14
// 1 = 竖屏 480x800 (书形), 与模拟器 emulator-750 一致; 锁屏时 lockScreen() 切横屏(rotation 0)
#define EPD_ROTATION 1
#define SUPPORT_PARTIAL_UPDATE true
// 开发板暂无按键, 占位 GPIO4 (不接线); TODO: 后期补按键后改成实际引脚
#define KEY_SWITCH 4
#define KEY_PIN_MODE INPUT_PULLUP
#define KEY_TRIGGER_LEVEL LOW
// 验证休眠: 无操作 5 分钟(SLEEP_TIMEOUT=300)深睡, 按 RST 键唤醒
// 注意: 深睡需要 USB 保持供电才能 RST 唤醒重新烧录/复位
#define SUPPORT_DEEP_SLEEP true
// 微雪驱动板无电池检测电路 (USB 供电), 电池逻辑(参考LiClock)预留给自绘驱动板
#define PIN_BATTERY_ADC -1   // 电池电压 ADC 引脚; -1 = 无
#define PIN_CHARGING -1      // 充电状态引脚(低=充电); -1 = 无
#define POWER_SOURCE_USB true // 无电池引脚时视为 USB 外接供电 (状态栏显示 USB 图标)
#endif

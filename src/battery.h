#pragma once
#include <Arduino.h>

// 锂电池 OCV(开路电压)-电量(SOC) 非线性映射 (单节 3.7V, 3.0~4.2V, 静态/轻载测得近似)
// 非线性: 两端(>4.0V / <3.5V)电压变化快, 3.6~3.9V 平台区变化平缓
// 换算(参考 LiClock): mv = analogRead(PIN_BATTERY_ADC) * BATTERY_ADC_FULL_MV / 4096
static const long   BATTERY_OCV_MV[]   = {3300, 3480, 3560, 3660, 3720, 3770, 3810, 3860, 3910, 3980, 4060, 4200};
static const int8_t BATTERY_OCV_PCT[]  = {0,    5,    10,   20,   30,   40,   50,   60,   70,   80,   90,   100};

// 由电池电压(mV)推算电量百分比(0~100), 分段线性插值, 1% 精度
inline int8_t batteryPercentFromMv(long mv) {
    if (mv >= 4200) return 100;
    if (mv <= 3300) return 0;
    const uint8_t n = sizeof(BATTERY_OCV_MV) / sizeof(BATTERY_OCV_MV[0]);
    for (uint8_t i = 1; i < n; i++) {
        if (mv < BATTERY_OCV_MV[i]) {
            return BATTERY_OCV_PCT[i - 1] +
                   (int8_t)((long)(mv - BATTERY_OCV_MV[i - 1]) * (BATTERY_OCV_PCT[i] - BATTERY_OCV_PCT[i - 1]) /
                            (BATTERY_OCV_MV[i] - BATTERY_OCV_MV[i - 1]));
        }
    }
    return 100;
}

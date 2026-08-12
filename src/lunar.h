#ifndef __LUNAR_H__
#define __LUNAR_H__

#include <stdint.h>
#include <stdbool.h>

// 公历日期转农历信息
struct LunarDate {
    int year;      // 农历年 (如 2026)
    int month;     // 农历月 (1-12)
    int day;       // 农历日 (1-30)
    bool isLeap;   // 是否闰月
    int term;      // 节气序号 0=非节气, 1=小寒 .. 24=冬至
    const char *festival; // 节假日名称, 无则 NULL
};

// 公历转农历, 成功返回 true
bool solarToLunar(int y, int m, int d, LunarDate &out);

// 农历月中文名 ("正".."腊"), 闰月由调用方加"闰"字
const char *lunarMonthCn(int month);
// 农历日中文名 ("初一".."三十")
const char *lunarDayCn(int day);
// 节气名称 (1-24), 非法返回 NULL
const char *termName(int term);

#endif // __LUNAR_H__

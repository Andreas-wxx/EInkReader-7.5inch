#include "lunar.h"
#include "lunar_data.h"

#include <cstddef>

// 农历月中文名
static const char *MONTH_CN[] = {"正", "二", "三", "四", "五", "六", "七", "八", "九", "十", "冬", "腊"};
// 农历日中文名
static const char *DAY_CN[] = {
    "初一", "初二", "初三", "初四", "初五", "初六", "初七", "初八", "初九", "初十",
    "十一", "十二", "十三", "十四", "十五", "十六", "十七", "十八", "十九", "二十",
    "廿一", "廿二", "廿三", "廿四", "廿五", "廿六", "廿七", "廿八", "廿九", "三十"
};

const char *lunarMonthCn(int month) {
    if (month < 1 || month > 12) return "";
    return MONTH_CN[month - 1];
}

const char *lunarDayCn(int day) {
    if (day < 1 || day > 30) return "";
    return DAY_CN[day - 1];
}

const char *termName(int term) {
    if (term < 1 || term > 24) return NULL;
    return TERM_NAMES[term - 1];
}

// 农历 y 年闰月月份, 0=无闰月
static int leapMonth(int y) {
    return LUNAR_INFO[y - 1900] & 0xf;
}

// 农历年 y 的总天数
static int lunarYearDays(int y) {
    int sum = 348;
    uint32_t info = LUNAR_INFO[y - 1900];
    for (uint16_t i = 0x8000; i > 0x8; i >>= 1) {
        sum += (info & i) ? 1 : 0;
    }
    return sum + (leapMonth(y) ? (LUNAR_INFO[y - 1900] & 0x10000 ? 30 : 29) : 0);
}

// 农历 y 年 m 月天数
static int monthDays(int y, int m) {
    return (LUNAR_INFO[y - 1900] & (0x10000 >> m)) ? 30 : 29;
}

// 公历 y/m/d 距 1900-1-31 的天数
static int dayOffset(int y, int m, int d) {
    static const int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int days = d;
    for (int i = 1900; i < y; i++) {
        days += 365 + ((i % 4 == 0 && i % 100 != 0) || i % 400 == 0 ? 1 : 0);
    }
    for (int i = 1; i < m; i++) {
        days += mdays[i - 1];
        if (i == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) days += 1;
    }
    return days - 31; // 1900-1-31 为基准
}

// 农历节日 (月, 日) -> 名称
static const char *lunarFestival(int m, int d) {
    if (m == 1 && d == 1) return "春节";
    if (m == 1 && d == 15) return "元宵节";
    if (m == 2 && d == 2) return "龙抬头";
    if (m == 5 && d == 5) return "端午节";
    if (m == 7 && d == 7) return "七夕";
    if (m == 8 && d == 15) return "中秋节";
    if (m == 9 && d == 9) return "重阳";
    if (m == 12 && d == 8) return "腊八节";
    if (m == 12 && d == 23) return "小年";
    return NULL;
}

// 公历节日 (月, 日) -> 名称
static const char *solarFestival(int m, int d) {
    if (m == 1 && d == 1) return "元旦";
    if (m == 2 && d == 14) return "情人节";
    if (m == 3 && d == 8) return "妇女节";
    if (m == 3 && d == 12) return "植树节";
    if (m == 5 && d == 1) return "劳动节";
    if (m == 6 && d == 1) return "儿童节";
    if (m == 7 && d == 1) return "建党节";
    if (m == 8 && d == 1) return "建军节";
    if (m == 9 && d == 10) return "教师节";
    if (m == 10 && d == 1) return "国庆节";
    if (m == 12 && d == 25) return "圣诞节";
    return NULL;
}

bool solarToLunar(int y, int m, int d, LunarDate &out) {
    if (y < 1900 || y > 2100) return false;
    if (y == 1900 && m == 1 && d < 31) return false;

    int offset = dayOffset(y, m, d);
    int i, temp = 0;
    // 计算农历年
    for (i = 1900; i < 2101 && offset > 0; i++) {
        temp = lunarYearDays(i);
        offset -= temp;
    }
    if (offset < 0) {
        offset += temp;
        i--;
    }
    out.year = i;

    int leap = leapMonth(out.year);
    bool isLeap = false;
    // 计算农历月
    for (i = 1; i < 13 && offset > 0; i++) {
        if (leap > 0 && i == leap + 1 && !isLeap) {
            --i;
            isLeap = true;
            temp = LUNAR_INFO[out.year - 1900] & 0x10000 ? 30 : 29;
        } else {
            temp = monthDays(out.year, i);
        }
        if (isLeap && i == leap + 1) isLeap = false;
        offset -= temp;
    }
    if (offset == 0 && leap > 0 && i == leap + 1) {
        if (isLeap) {
            isLeap = false;
        } else {
            isLeap = true;
            --i;
        }
    }
    if (offset < 0) {
        offset += temp;
        --i;
    }
    out.month = i;
    out.day = offset + 1;
    out.isLeap = isLeap;

    // 节气: 当天是否处于 24 节气
    out.term = 0;
    for (int t = 0; t < 24; t++) {
        if (TERM_DAYS[y - 1900][t] == d) {
            out.term = t + 1;
            break;
        }
    }

    // 节假日: 优先公历节日 (国庆/元旦), 否则农历节日, 否则节气 (清明既是节气也是节日)
    out.festival = solarFestival(m, d);
    if (!out.festival) out.festival = lunarFestival(out.month, out.day);
    if (!out.festival && out.term == 5) out.festival = "清明节"; // 清明
    return true;
}

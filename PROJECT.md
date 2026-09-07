# E-Ink Reader 7.5" 项目备忘（PROJECT.md）

> 给妈妈做的 7.5 寸墨水屏阅读器。本文档记录**整体框架 + 当前进度**，供更换开发设备/工具后快速上手。
> 更新本文件时**必须带时间戳**（格式 `YYYY-MM-DD HH:MM`）。

---

## 一、项目一句话

基于 **ESP32-WROOM-32D-N16 + 微雪 7.5" V2 黑白墨水屏（裸屏玻璃）** 的墨水屏阅读器：
支持在线（WiFi 天气/下载电子书）与离线（DS3231 时钟 / 本地书库 / AHT20）双模式；
首页为时钟+天气+应用 Dock，右侧"手柄"式机身（PCB 区），仿文石 leaf 形态。

---

## 二、硬件

### 2.1 屏幕（微雪 7.5 V2 裸屏）
- 型号：GDEY075T7 / 控制器 **UC8179**；分辨率 800×480；**支持局部刷新(约0.45s)与 4 级灰阶**
- 玻璃外形 170.2 × 111.2 × **1.18mm**（含 COG）；显示区 163.2 × 97.92
- FPC **24pin**：MCU 只接 8 根（BUSY/RST/DC/CS/CLK/SDA + VDD/VDDIO/VSS，BS 拉低=4线SPI）；
  其余（GDR/RESE/VSH/VGH/VSL/VGL/VCOM/VDD18/VOTP 等）是面板电源脚，**自绘板必须照抄微雪驱动板原理图**的外围电路
- 屏幕脆弱（玻璃），外壳需：前框开窗压边 + 0.8~1mm 刚性背板支撑 + 薄双面胶贴合（**不可用海绵/棉花当支撑**）

### 2.2 主控与验证板
- 最终主控：**ESP32-WROOM-32D-N16**（16MB Flash）；与验证用 **WROOM-32E 引脚完全兼容**，已验证引脚可直接复用
- 验证板：微雪 e-Paper ESP32 Driver Board **Rev3**（env: `esp32-waveshare-750`）

### 2.3 引脚分配（屏幕部分已验证 ✅；其余为自绘板规划 ⚠️，待定稿后同步进 board_config.h）
| 功能 | ESP32 GPIO | 软件宏 | 状态 |
|---|---|---|---|
| 屏 CLK | 13 | `EPD_CLK` | ✅ 已验证 |
| 屏 MOSI | 14 | `EPD_MOSI` | ✅ |
| 屏 CS | 15 | `EPD_CS` | ✅ |
| 屏 DC | 27 | `EPD_DC` | ✅ |
| 屏 RST | 26 | `EPD_RST` | ✅ |
| 屏 BUSY | 25 | `EPD_BUSY` | ✅ |
| **屏幕电源控制** | **23** | `EPD_PWR`（规划） | ⚠️ 独立LDO开关，平时常开，仅关机/深度休眠断电 |
| UART0 TX/RX | 1 / 3 | → CH340N | ⚠️ 烧录口 |
| 电池电压 ADC | 34 | `PIN_BATTERY_ADC` | ⚠️ |
| 充电/USB 检测 | 32 | `PIN_CHARGING` | ⚠️ |
| AHT20 | SDA21 / SCL22 | — | ⚠️ 温湿度 |
| DS3231 RTC | 同 I2C（SDA21/SCL22） | — | ⚠️ 高精度时钟 + CR2032 |
| microSD | SCK13/MOSI14/MISO19/CS16 | — | ⚠️ 与屏共 SPI 总线、分 CS |
| 翻页键↑ / ↓ / 确认 | 4 / 18 / 17 | `KEY_SWITCH` 等 | ⚠️ |
| 空闲可扩展 | 5,12,23,33,35,36,39 | — | 35/36/39 输入专用需外上拉 |

### 2.4 电源拓扑（规划，参考微雪驱动板）
- USB 5V → 防反接 → 常电 LDO(3.3V，ESP32 等) ；另路 → P-MOS 可控开关 → **屏幕独立 LDO** → EPD_VCC
- 屏幕供电策略（两级）：平时/锁屏**常开**（保留控制器 RAM 做局部刷新）；**仅关机/深度休眠才断电**（唤醒整页重建）
- 电池：3.7V 薄聚合物；充电/电源路径 IC（带 power path，如 IP5306/ETA9640）+ 3.3V LDO

### 2.5 外壳（OpenSCAD，见 `D:\_OpenSCAD\EInkReader7in5\`）
- 仿文石 leaf：屏幕外露（前框开窗压边），**左薄**（屏幕+背板+电池）/ **右厚手柄**（PCB+按键，握持）
- 按键：前侧右操作区上下 2 翻页键（拇指）+ 右侧面上部确认/电源键；顶部 ESP32 天线开窗；底部 TypeC
- 版本：`shell_v0.2.scad`（早期）→ `shell_v0.3.scad`（玻璃外露版，待完善，保留各版便于微调）
- 嘉立创 3D 打印收 **STL**；嘉立创 PCB 免费打样限 **100×100mm** 内

---

## 三、软件（PlatformIO）

### 3.1 环境（platformio.ini）
| env | 平台 | 用途 |
|---|---|---|
| `emulator-750` | native(SDL) | PC 模拟器，代码与真机共享同一份 src |
| `esp32-waveshare-750` | espressif32 / esp32dev | 微雪 Rev3 验证板（当前主力真机） |
| `smpl-v2-0213-x` / `coreboard-v1-2` | ESP8266/ESP32-C3 | 旧项目遗留，勿动 |

> 模拟器/真机**共用同一份源码**，无需复制；`emulator.cpp` 有 `#ifdef NATIVE` 保护。

### 3.2 源码结构（src/）
- `main.cpp`：setup/loop、页面管理(addPage/pages)、天气缓存、锁屏与 **light-sleep 待机 lowPowerIdle**、WebServer 配置
- `ui_custom.cpp`：7.5 寸全部 UI——首页(时间+天气+应用Dock)、锁屏(休眠)界面、右侧应用栏、状态栏、图标(xbm)
- `draw.cpp`：`startDraw/endDraw`（显示封装）、文字/图标绘制
- `config.h`：全局配置宏（UI 分辨率、天气 key、休眠时长、电池检测宏等）
- `board_config.h`：各板型引脚宏（SMPL_V2 / COREBOARD / NATIVE / **WAVESHARE_DRIVER_REV3**）
- `API.hpp`：天气/HTTP 网络层（native 走 curl，ESP32 走 WiFiClientSecure）
- `lunar.*`：农历/节气/节假日；`util.*`、`bitmap.h`、各 `u8g2_*` 字体
- 其它 `ui_lg/md/sm/xl/xs.cpp`：旧尺寸布局（未用，占少量 flash）

### 3.3 关键机制（改前必读）
1. **驱动**：GxEPD2 `GxEPD2_750_GDEY075T7`（UC8179）。apply_patches.py 对 espressif32 环境**禁用 GxEPD2 内部 `SPI.begin()`**（微雪用非默认引脚 13/14/15，SPI 由 setup `SPI.begin(EPD_CLK,-1,EPD_MOSI,EPD_CS)` 接管）
2. **局部刷新**：`refreshHomeClock()`(首页顶)/`refreshLockClock()`(锁屏) 用 GxEPD2_BW `displayWindow()` 刷小窗口；
   **endDraw 不 hibernate**（hibernate 掉电清控制器 RAM，破坏 fast partial 差分）
3. **休眠待机**：`lowPowerIdle()`（ESP32）light-sleep 周期唤醒：每分刷锁屏时间(局部)、每 2h 刷天气、约 30 次局部后整页刷新清残影；按键唤醒回首页
4. **电池/状态栏**：`getBatteryLevel()/isCharging()` 支持板级宏（PIN_BATTERY_ADC/PIN_CHARGING/POWER_SOURCE_USB，参考 LiClock）；充电显示闪电+100%，否则 5 档电池+实时百分比
5. 休眠界面跨天日期局部刷新窗口 **y 260~292**（不能延伸到 ≥293，否则削掉温度曲线最高温标注的"头"）

### 3.4 常用命令
```
pio run -e emulator-750            # 编译 PC 模拟器（.pio\build\emulator-750\program.exe）
pio run -e esp32-waveshare-750 -t upload --upload-port COM12   # 烧录真机
pio device monitor -p COM12 -b 115200
```

---

## 四、产品需求（已确认）
- **双模式**：在线(WiFi 天气/在线下载书) + 离线(必须能看时间+看书)
- 离线时间源 = **DS3231**（CR2032 后备，高精度守时；在线 NTP 自动校准写入 DS3231，离线设置菜单手动校准）
- 离线时电子书(本地 TF 卡) 与 **AHT20** 温湿度必须可用
- 设置菜单（参考 LiClock appSettings）：字体大小 / WiFi 配置(SSID+密码 与 smartconfig 两法) / 时间日期校准 / 系统状态(内存、TF 占用、挂载检测)
- 屏幕供电两级策略（见 2.4）
- 外壳：玻璃外露+边框保护，右手柄，见 2.5

---

## 五、进度日志（倒序，最新在上）

### 2026-09-07
- 建立本项目备忘；确认硬件与引脚规划（含 EPD_PWR=23 两级供电、AHT20/DS3231/TF 卡外设、离线模式需求）
- 外壳进入 OpenSCAD 设计（v0.2→v0.3 玻璃外露版），保留各版本便于微调
- 澄清：liclock `SDVDD_CTRL`=TF卡电源（非屏幕）；屏幕采用"常供电+powerOff"支持局部刷新，断电开关仅关机用

### 2026-09-06
- ESP32 真机验证完成：屏幕显示/天气/时间分钟级局部刷新/light-sleep 周期唤醒待机（每分刷时间+2h天气+30次清残影）
- 状态栏重做：WiFi、5 档电池+百分比、充电=闪电+100%；休眠界面跨天日期窗口避开温度标注
- 深睡前切横屏低功耗界面；修复局部刷新被强制全刷的 bug（去掉重复 init / hibernate）
- 完成 GitHub `codex` 分支推送

### 2026-09-05（含前）
- PC 模拟器调通；首页/锁屏 UI、右侧应用栏、状态栏逐步定型
- 真机移植：新增 `esp32-waveshare-750` 环境、板型引脚、GxEPD2 SPI 补丁、arduino-esp32 框架离线安装

---

## 六、待办（Roadmap）
- [ ] 自绘 PCB 原理图/布线（屏幕 FPC 外围电路照抄微雪驱动板原理图）
- [ ] `board_config.h` 新增自绘板板型（同步本表引脚 + EPD_PWR）
- [ ] DS3231 接入 + 时间源切换（NTP 优先 / DS3231 离线）+ 设置校准页
- [ ] AHT20 温湿度接入首页/锁屏
- [ ] TF 卡(SD) + 电子书引擎（.txt/.epub 解析、翻页、书签）+ 中文字体按需加载
- [ ] 设置菜单（字体/WiFi/时间/系统状态，参考 LiClock）
- [ ] WiFi 配置两种方式（SSID+密码 & SmartConfig）
- [ ] 离线 UI 细节（用户待补充需求）
- [ ] 外壳 v0.3 完善 → 嘉立创打印验证

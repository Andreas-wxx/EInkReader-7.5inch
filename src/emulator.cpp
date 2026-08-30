#ifdef NATIVE

#include <chrono>
#include <thread>
#include <atomic>
#include <SDL3/SDL.h>
#include <Arduino.h>

#define private public
#define protected public
#include "draw.h"
#undef private
#undef protected

// 模拟器屏幕旋转方向：
// 0: 竖屏
// 1: 横屏(顺时针90度)
// 2: 倒置竖屏(顺时针180度)
// 3: 倒置横屏(顺时针270度)
#ifndef EMULATOR_ROTATION
#define EMULATOR_ROTATION 3
#endif

// 模拟器屏幕窗口的放大倍数
#ifndef EMULATOR_SCALE
#define EMULATOR_SCALE 2
#endif

// 模拟器屏幕刷新率
#ifndef EMULATOR_FPS
#define EMULATOR_FPS 20
#endif

extern EPD_CLASS epd;
extern bool keyPressed;
extern int selectedApp;
extern volatile bool appBarOpen;
extern volatile bool emuRequestOpenApp;
extern void drawAppBar(EPD_CLASS &epd, U8G2_FOR_ADAFRUIT_GFX &u8g2);
extern U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
extern void refreshPage();
extern volatile bool screenLocked;
extern volatile bool emuRequestLock;
extern volatile bool emuRequestUnlock;
extern void lockScreen();
extern void unlockScreen();

template<typename GxEPD2_Class>
uint32_t get_color(GxEPD2_Class& epd, int x, int y);

template<typename GxEPD2_Type, const uint16_t page_height>
uint32_t get_color(GxEPD2_BW<GxEPD2_Type, page_height>& epd, int x, int y) {
    int byte_idx = (x / 8) + y * (epd.WIDTH / 8);
    uint8_t bit_mask = 1 << (7 - (x % 8));

    bool is_black = !(epd._buffer[byte_idx] & bit_mask);

    uint32_t color = 0xFFFFFFFF;
    if (is_black) color = 0xFF000000;      // 黑
    return color;
}

template<typename GxEPD2_Type, const uint16_t page_height>
uint32_t get_color(GxEPD2_3C<GxEPD2_Type, page_height>& epd, int x, int y) {
    int byte_idx = (x / 8) + y * (epd.WIDTH / 8);
    uint8_t bit_mask = 1 << (7 - (x % 8));

    bool is_black = !(epd._black_buffer[byte_idx] & bit_mask);
    bool is_color = !(epd._color_buffer[byte_idx] & bit_mask);

    uint32_t color = 0xFFFFFFFF;
    if (is_black) color = 0xFF000000;      // 黑
    else if (is_color) color = 0xFFFF0000; // 红/黄
    return color;
}

int main(int argc, char* argv[]) {
    // 使日志能在 pio 的输出窗口上实时打印
    setbuf(stdout, NULL);
    // Window size must consider EPD_ROTATION: setup() runs later in a thread,
    // so apply rotation here first to get the correct rotated dimensions.
    epd.setRotation(EPD_ROTATION);

    // 墨水屏尺寸
    int width = epd.WIDTH;
    int height = epd.HEIGHT;

    // 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Window* window = SDL_CreateWindow("EInkAssistant Emulator | F2 switch page | F5 refresh | F6 lock | Enter unlock",
                                            480 * EMULATOR_SCALE, 800 * EMULATOR_SCALE, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* texture = nullptr;
    uint32_t* pixels = nullptr;
    int win_width = 0, win_height = 0;

    // 锁屏长按检测
    bool enterDown = false;
    uint64_t enterDownTime = 0;

    // 无操作自动锁屏: 默认 5 分钟 (可用 EMU_IDLE_SEC 覆盖, 便于调试)
    uint64_t lastInputTime = SDL_GetTicks();
    uint32_t idleSec = 300;
    if (getenv("EMU_IDLE_SEC") != nullptr) idleSec = (uint32_t) atoi(getenv("EMU_IDLE_SEC"));

    // 截图: F9 手动保存; EMU_AUTOSHOT=1 时自动: 锁屏截图 -> 解锁 -> 首页截图 (调试用)
    bool autoShot = getenv("EMU_AUTOSHOT") != nullptr;
    bool autoLockDone = false, autoShotDone = false, autoHomeDone = false, autoHomeShotDone = false;
    uint64_t autoStart = SDL_GetTicks();
    if (autoShot) printf("[EMU] EMU_AUTOSHOT enabled\n");

    // 调试: EMU_APPBAR=1 时启动后自动呼出右侧应用栏 (验证应用栏 UI, 无需按键)
    bool autoAppBar = getenv("EMU_APPBAR") != nullptr;
    bool autoAppBarDone = false;
    if (autoAppBar) printf("[EMU] EMU_APPBAR enabled\n");

    // 启动 Arduino 线程
    std::atomic<bool> running{true};
    std::atomic<bool> refresh{false};
    std::atomic<bool> dockRefresh{false};
    std::thread arduino_thread([&running, &refresh, &dockRefresh]() {
        setup();
        while (running) {
            loop();
            if (refresh) {
                refreshPage();
                refresh = false;
            }
            if (dockRefresh) {
                // 只重绘右侧应用栏 (不清屏), 模拟墨水屏局部刷新
                drawAppBar(epd, u8g2Fonts);
                dockRefresh = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    // SDL 主循环
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                lastInputTime = SDL_GetTicks(); // 记录最后操作时间 (无操作自动锁屏)
                if (screenLocked) {
                    // 锁屏: 任意键按下开始计时, 长按 2 秒唤醒
                    if (!enterDown) {
                        enterDown = true;
                        enterDownTime = SDL_GetTicks();
                    }
                } else if (event.key.key == SDLK_F2) {
                    keyPressed = true;
                } else if (event.key.key == SDLK_F5) {
                    refresh = true;
                } else if (event.key.key == SDLK_LEFT) {
                    selectedApp = (selectedApp + 3) % 4; // 向左选上一个应用
                    dockRefresh = true;
                } else if (event.key.key == SDLK_RIGHT) {
                    selectedApp = (selectedApp + 1) % 4; // 向右选下一个应用
                    dockRefresh = true;
                } else if (event.key.key == SDLK_F6) {
                    emuRequestLock = true; // 进入锁屏(模拟)
                } else if (event.key.key == SDLK_DOWN || event.key.key == SDLK_RETURN) {
                    // 只记录首次按下时间: SDL 长按时会重复发 KEY_DOWN, 否则计时被不断重置
                    if (!enterDown) {
                        enterDown = true; // 确认键按下 (下箭头=确认, 对应硬件中键位置)
                        enterDownTime = SDL_GetTicks();
                    }
                } else if (event.key.key == SDLK_F9) {
                    // F9: 保存当前画面为 BMP (调试用)
                    if (texture) {
                        SDL_Surface* surf = SDL_CreateSurface(win_width, win_height, SDL_PIXELFORMAT_ARGB8888);
                        memcpy(surf->pixels, pixels, (size_t) win_width * win_height * sizeof(uint32_t));
                        SDL_SaveBMP(surf, "emu_shot.bmp");
                        SDL_DestroySurface(surf);
                        printf("Screenshot saved to doc/emu_shot.bmp\n");
                    }
                }
            } else if (event.type == SDL_EVENT_KEY_UP) {
                if (screenLocked) {
                    enterDown = false; // 锁屏: 任意键松开
                } else if (event.key.key == SDLK_DOWN || event.key.key == SDLK_RETURN) {
                    if (enterDown) {
                        uint64_t holdMs = SDL_GetTicks() - enterDownTime;
                        enterDown = false;
                        if (holdMs >= 1500) {
                            // 长按(>=1.5s): 收起应用栏, 不进入应用
                            if (appBarOpen) {
                                appBarOpen = false;
                                refresh = true; // 整页重绘恢复完整首页
                                printf("[EMU] app bar closed (long press)\n");
                            }
                        } else {
                            // 短按: 呼出应用栏 / 打开选中应用
                            if (appBarOpen) {
                                emuRequestOpenApp = true; // 由 Arduino 线程执行, 避免跨线程操作 epd
                                printf("[EMU] open app %d\n", selectedApp);
                            } else {
                                appBarOpen = true; // 呼出右侧应用栏
                                dockRefresh = true;
                                printf("[EMU] app bar opened (confirm)\n");
                            }
                        }
                    }
                }
            }
        }

        // 调试: EMU_APPBAR=1 时 1 秒后自动呼出应用栏
        if (autoAppBar && !autoAppBarDone && !screenLocked && SDL_GetTicks() - autoStart > 1000) {
            appBarOpen = true;
            dockRefresh = true;
            autoAppBarDone = true;
            printf("[EMU] auto app bar opened\n");
        }

        // 锁屏: 任意键长按 2 秒 -> 唤醒
        if (enterDown && screenLocked && SDL_GetTicks() - enterDownTime > 2000) {
            emuRequestUnlock = true;
            enterDown = false;
        }

        // 非锁屏: 应用栏打开时, 确认键长按 1.5 秒 -> 收起应用栏 (SDL 长按会重复发 KEY_DOWN, 这里兜底)
        if (enterDown && !screenLocked && appBarOpen && SDL_GetTicks() - enterDownTime > 1500) {
            appBarOpen = false;
            enterDown = false;
            refresh = true;
            printf("[EMU] app bar closed (long press, loop)\n");
        }

        // 无操作 idleSec 秒 -> 自动进入低功耗(锁屏)
        if (!screenLocked && SDL_GetTicks() - lastInputTime > (uint64_t) idleSec * 1000) {
            emuRequestLock = true;
            lastInputTime = SDL_GetTicks(); // 防止重复触发
            printf("[EMU] idle %u s, auto lock\n", idleSec);
        }

        // EMU_AUTOSHOT=1: 2 秒后自动进锁屏, 再 2 秒后自动截图 (调试用)
        if (autoShot && !autoLockDone && SDL_GetTicks() - autoStart > 2000) {
            emuRequestLock = true;
            autoLockDone = true;
            printf("[EMU] auto lock requested\n");
        }
        if (autoShot && !autoShotDone && screenLocked && SDL_GetTicks() - autoStart > 4000) {
            if (texture) {
                SDL_Surface* surf = SDL_CreateSurface(win_width, win_height, SDL_PIXELFORMAT_ARGB8888);
                memcpy(surf->pixels, pixels, (size_t) win_width * win_height * sizeof(uint32_t));
                bool ok = SDL_SaveBMP(surf, "emu_lock.bmp");
                printf("[EMU] auto lock screenshot: %s\n", ok ? "OK" : "FAIL");
                SDL_DestroySurface(surf);
            }
            autoShotDone = true;
        }
        // 锁屏截图后: 6 秒自动解锁回首页
        if (autoShot && autoShotDone && !autoHomeDone && SDL_GetTicks() - autoStart > 6000) {
            emuRequestUnlock = true;
            autoHomeDone = true;
            printf("[EMU] auto unlock requested\n");
        }
        // 首页显示后: 8 秒截首页图
        if (autoShot && autoHomeDone && !autoHomeShotDone && !screenLocked && SDL_GetTicks() - autoStart > 8000) {
            if (texture) {
                SDL_Surface* surf = SDL_CreateSurface(win_width, win_height, SDL_PIXELFORMAT_ARGB8888);
                memcpy(surf->pixels, pixels, (size_t) win_width * win_height * sizeof(uint32_t));
                bool ok = SDL_SaveBMP(surf, "emu_home.bmp");
                printf("[EMU] auto home screenshot: %s\n", ok ? "OK" : "FAIL");
                SDL_DestroySurface(surf);
            }
            autoHomeShotDone = true;
        }

        // 动态窗口方向: 锁屏(rotation 0) 横屏 800x480, 首页(rotation 1) 竖屏 480x800
        int display_rot = (epd.getRotation() == 0) ? 0 : (EMULATOR_ROTATION % 4);
        int new_ww = (display_rot % 2 == 1) ? height : width;
        int new_wh = (display_rot % 2 == 1) ? width : height;
        if (new_ww != win_width || new_wh != win_height || texture == nullptr) {
            win_width = new_ww;
            win_height = new_wh;
            SDL_SetWindowSize(window, win_width * EMULATOR_SCALE, win_height * EMULATOR_SCALE);
            if (texture) SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, win_width, win_height);
            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART);
            if (pixels) delete[] pixels;
            pixels = new uint32_t[win_width * win_height];
        }

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int win_x = x;
                int win_y = y;
                if (display_rot == 1) {
                    win_x = height - 1 - y;
                    win_y = x;
                } else if (display_rot == 2) {
                    win_x = width - 1 - x;
                    win_y = height - 1 - y;
                } else if (display_rot == 3) {
                    win_x = y;
                    win_y = width - 1 - x;
                }
                pixels[win_y * win_width + win_x] = get_color(epd, x, y);
            }
        }

        SDL_UpdateTexture(texture, nullptr, pixels, win_width * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        SDL_Delay(1000 / EMULATOR_FPS);
    }

    // 等待 Arduino 线程退出
    arduino_thread.join();

    // 释放资源
    delete[] pixels;
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

#endif // NATIVE

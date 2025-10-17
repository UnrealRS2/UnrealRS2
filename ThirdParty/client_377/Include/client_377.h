#pragma once
#include <functional>
#include <string>

#ifdef CLIENT_API_EXPORTS
#define CLIENT_API __declspec(dllexport)
#else
#define CLIENT_API __declspec(dllimport)
#endif

extern "C" {
    namespace api {

        using PlatformSetColorCallback = void(*)(int color);
        using PlatformDrawRectCallback = void(*)(int x, int y, int w, int h);

        struct PlatformCallbacks {
            PlatformSetColorCallback setColor = nullptr;
            PlatformDrawRectCallback drawRect = nullptr;
        };

        inline PlatformCallbacks g_PlatformCallbacks = PlatformCallbacks();

        CLIENT_API inline void platform_set_color(int color) {
            if (g_PlatformCallbacks.setColor)
                g_PlatformCallbacks.setColor(color);
        }

        CLIENT_API inline void platform_draw_rect(int x, int y, int w, int h) {
            if (g_PlatformCallbacks.drawRect)
                g_PlatformCallbacks.drawRect(x, y, w, h);
        }

        using PrintCallback = std::function<void(const char*)>;
        inline PrintCallback printCallback;
        CLIENT_API void SetPrintCallback(PrintCallback callback);

        using DrawCallback = std::function<void(std::vector<int> pixels, int x, int y)>;
        inline DrawCallback drawCallback;
        CLIENT_API void SetDrawCallback(DrawCallback callback);

        CLIENT_API void Init();
    }
}


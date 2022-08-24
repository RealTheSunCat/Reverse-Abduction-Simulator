#pragma once

#include <chrono>
#include <functional>
#include <iomanip>

#include "Platform.h"

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#undef APIENTRY
#include "Windows.h"
#undef APIENTRY

// stolen from https://github.com/ikalnytskyi/termcolor/blob/master/include/termcolor/termcolor.hpp#L567, thanks!
//! Change Windows Terminal colors attribute. If some
//! parameter is `-1` then attribute won't changed.
inline void win_change_attributes(const int foreground)
{
    // yeah, i know.. it's ugly, it's windows.
    static WORD defaultAttributes = 0;

    HANDLE hTerminal = GetStdHandle(STD_OUTPUT_HANDLE);

    // save default terminal attributes if it unsaved
    if (!defaultAttributes)
    {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (!GetConsoleScreenBufferInfo(hTerminal, &info))
            return;
        defaultAttributes = info.wAttributes;
    }

    // restore all default settings
    if (foreground == -1)
    {
        SetConsoleTextAttribute(hTerminal, defaultAttributes);
        return;
    }

    // get current settings
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(hTerminal, &info))
        return;

    if (foreground != -1)
    {
        info.wAttributes &= ~(info.wAttributes & 0x0F);
        info.wAttributes |= static_cast<WORD>(foreground);
    }

    info.wAttributes &= ~(info.wAttributes & 0xF0);
    info.wAttributes |= static_cast<WORD>(0);

    SetConsoleTextAttribute(hTerminal, info.wAttributes);
}

#define CHANGE_COLOR(col) win_change_attributes((col) == 0 ? -1 : (col));
#else

    #ifdef PLATFORM_LINUX
        #define CHANGE_COLOR(col) printf("\033[%im", (col))
    #else
        #define CHANGE_COLOR(col) /* unsupported :/ */
    #endif

#endif

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <iostream>
#include <ctime>

template <typename T>
class Queue
{
public:
    bool empty()
    {
        std::unique_lock<std::mutex> mlock(mutex);
        return queue.empty();
    }

    T pop()
    {
        std::unique_lock<std::mutex> mlock(mutex);
        while (queue.empty())
        {
            cond.wait(mlock);
        }
        auto item = std::move(queue.front());
        queue.pop();
        return item;
    }

    void pop(T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex);
        while (queue.empty())
        {
            cond.wait(mlock);
        }
        item = std::move(queue.front());
        queue.pop();
    }

    void push(const T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex);
        queue.push(item);
        mlock.unlock();
        cond.notify_one();
    }

    void push(T&& item)
    {
        std::unique_lock<std::mutex> mlock(mutex);
        queue.push(std::move(item));
        mlock.unlock();
        cond.notify_one();
    }

private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable cond;
};

inline Queue<std::function<void()>> loggerQueue;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
// TODO this function is EVIL. segfault on Linux. Why??
template <typename T>
static decltype(auto) printf_transform(T const& arg)
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        return arg.c_str();
    }
    else // TODO add this back maybe
    {
        //static_assert(std::is_trivially_copyable_v<T>);
        return arg;
    }
}
#pragma GCC diagnostic pop

inline auto thread_safe_time(const std::time_t& time) {
    static std::mutex mut;
    std::lock_guard<std::mutex> lock(mut);

#ifdef _WIN32
    auto localtime_ret = std::localtime(&time);;
#else
    tm* localtime_ret = new tm();
    localtime_r(&time, localtime_ret);
#endif
    return localtime_ret;
}

#ifdef PLATFORM_ANDROID
#include <android/log.h>

#ifndef LOG_TAG
    #define LOG_TAG "logtest"
    #define slogd(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#endif
#endif

struct smart_printf {
    template <typename ...Ts>
    void operator()(Ts const& ...args) const
    {
        char time_buf[128];
        std::time_t t = std::time(nullptr);
        const auto time_strlen = std::strftime(time_buf, sizeof(time_buf), "[%Y/%m/%d, %H:%M:%S] ", thread_safe_time(t));

        if (time_strlen == 0) {
            std::cerr << "Failed to retrieve date/time.\n";
            return;
        }
#ifdef PLATFORM_ANDROID
        slogd("%s", time_buf);
        slogd(printf_transform(args)...);
        slogd("\n");
#else
        std::cout << time_buf;
        printf(printf_transform(args)...);
        putchar('\n');
#endif
    }
};

#define LOG(...) /*loggerQueue.push([args=std::make_tuple(__VA_ARGS__)] { */std::apply(smart_printf{}, std::make_tuple(__VA_ARGS__))/* })*/

#ifdef _DEBUG 
#define LOG_DEBUG(...) /*loggerQueue.push([args=std::make_tuple(__VA_ARGS__)] {*/ CHANGE_COLOR(35); /* set color to magenta */\
        std::apply(smart_printf{}, std::make_tuple(__VA_ARGS__)); \
        putchar('\n'); \
        CHANGE_COLOR(0)/*})*/

#define PROFILE Util::Timer timer_##__COUNTER__(__func__)
#else
#define LOG_DEBUG(...)
#define PROFILE
#endif

#define LOG_ERROR(...) /*loggerQueue.push([args=std::make_tuple(__VA_ARGS__)] {*/ CHANGE_COLOR(4); /* red error color */\
        std::apply(smart_printf{}, std::make_tuple(__VA_ARGS__)); \
        CHANGE_COLOR(0)/*;})*/

#define LOG_INFO(...) /*loggerQueue.push([args=std::make_tuple(__VA_ARGS__)] {*/ CHANGE_COLOR(34); /* green info color */\
        std::apply(smart_printf{}, std::make_tuple(__VA_ARGS__)); \
        CHANGE_COLOR(0)/*;})*/

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&) = delete;   \
    TypeName& operator=(const TypeName&) = delete

#define DISALLOW_COPY(TypeName) \
    TypeName(const TypeName&) = delete

#define DISALLOW_ASSIGN(TypeName) \
    TypeName& operator=(const TypeName&) = delete

#define BIT(x) (1 << (x))

#define BIND_EVENT_FUNC(function) std::bind(&function, this, std::placeholders::_1)

#define GET_ITEM(item) Outrospection::get().itemRegistry.get(item)
#define ITEM_EXISTS(item) Outrospection::get().itemRegistry.has(item)

#ifndef _LOGGING_H_
#define _LOGGING_H_

#define __FILENAME__ (__builtin_strchr(__FILE__, '/') + 1)

#define log_debug(msg, ...) Log::debug(__LINE__, __FILENAME__, msg, ## __VA_ARGS__)
#define log_info(msg, ...) Log::info(__LINE__, __FILENAME__, msg, ## __VA_ARGS__)
#define log_warn(msg, ...) Log::warn(__LINE__, __FILENAME__, msg, ## __VA_ARGS__)
#define log_error(msg, ...) Log::error(__LINE__, __FILENAME__, msg, ## __VA_ARGS__)
#define log_fatal(msg, ...) Log::fatal(__LINE__, __FILENAME__, msg, ## __VA_ARGS__)

#define log_assert(cond, msg, ...) \
    do { \
        if (!(cond)) { \
            Log::fatal(__LINE__, __FILENAME__, msg, ## __VA_ARGS__); \
        } \
    } while (0)

class Log {
private:
    static int log_level; // 0: debug, 1: info, 2: warn, 3: error, 4: fatal
    static FILE* fp_s;
    static std::mutex log_mutex;

    static const size_t TIME_BUF_SIZE = 24;

private:
    static void log(int level, int line, const char* file, const char* fmt, va_list args);

public:
    enum Level {
        FATAL = 0,
        ERROR = 1,
        WARN  = 2,
        INFO  = 3,
        DEBUG = 4
    };

    static void set_log_file(FILE* fp);
    static void set_log_level(int level);

    static void debug(int line, const char* file, const char* msg, ...);
    static void info(int line, const char* file, const char* msg, ...);
    static void warn(int line, const char* file, const char* msg, ...);
    static void error(int line, const char* file, const char* msg, ...);
    static void fatal(int line, const char* file, const char* msg, ...);
};

#endif // _LOGGING_H_

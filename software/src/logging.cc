#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <mutex>

#include "logging.h"

int Log::log_level = Log::DEBUG;
FILE* Log::fp_s = stdout;
std::mutex Log::log_mutex;

void Log::set_log_level(int level) {
    std::lock_guard<std::mutex> lock(log_mutex);
    log_level = level;
}

void Log::set_log_file(FILE* fp) {
    log_assert(fp_s != nullptr, "Log file is not opened");
    std::lock_guard<std::mutex> lock(log_mutex);
    fp_s = fp;
}

void Log::debug(int line, const char* file, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    log(Log::DEBUG, line, file, msg, args);
    va_end(args);
}

void Log::info(int line, const char* file, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    log(Log::INFO, line, file, msg, args);
    va_end(args);
}

void Log::warn(int line, const char* file, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    log(Log::WARN, line, file, msg, args);
    va_end(args);
}

void Log::error(int line, const char* file, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    log(Log::ERROR, line, file, msg, args);
    va_end(args);
}

void Log::fatal(int line, const char* file, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    log(Log::FATAL, line, file, msg, args);
    va_end(args);
    fflush(fp_s);
    abort();
}

void make_int(char* buf, int val, int digits) {
    char* p = buf + digits;
    for (int i = 0; i < digits; i++) {
        int d = val % 10;
        val /= 10;
        p--;
        *p = '0' + d;
    }
}

void current_time(char* buf) {
    time_t sec_since_epoch = time(nullptr);
    struct tm tm;
    localtime_r(&sec_since_epoch, &tm);
    make_int(buf, tm.tm_year + 1900, 4);
    buf[4] = '-';
    make_int(buf + 5, tm.tm_mon + 1, 2);
    buf[7] = '-';
    make_int(buf + 8, tm.tm_mday, 2);
    buf[10] = ' ';
    make_int(buf + 11, tm.tm_hour, 2);
    buf[13] = ':';
    make_int(buf + 14, tm.tm_min, 2);
    buf[16] = ':';
    make_int(buf + 17, tm.tm_sec, 2);
    buf[19] = '.';
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    make_int(buf + 20, tv.tv_usec / 1000, 3);
    buf[23] = '\0';
}

void Log::log(int level, int line, const char* file, const char* fmt, va_list args) {
    static const char* level_str[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    assert(level >= 0 && level <= Log::DEBUG);
    assert(file != nullptr);

    if (level > log_level) return;

    char time_buf[TIME_BUF_SIZE];
    current_time(time_buf);

    char log_buf[4096];
    size_t offset = 0;
    offset += snprintf(log_buf + offset, sizeof(log_buf) - offset, "%s ", level_str[level]);
    offset += snprintf(log_buf + offset, sizeof(log_buf) - offset, "[%s:%d] ", file, line);
    offset += snprintf(log_buf + offset, sizeof(log_buf) - offset, "%s ", time_buf);
    offset += vsnprintf(log_buf + offset, sizeof(log_buf) - offset, fmt, args);
    offset += snprintf(log_buf + offset, sizeof(log_buf) - offset, "\n");

    fprintf(fp_s, "%s", log_buf);
}


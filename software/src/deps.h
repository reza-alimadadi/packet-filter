#ifndef _DEPS_H_
#define _DEPS_H_

#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

#include "logging.h"
class dummy_class {
public:
    dummy_class() {
#ifdef LOG_LEVEL_DEBUG
        Log::set_log_level(Log::DEBUG);
#else
        Log::set_log_level(Log::INFO);
#endif
    }
};
static dummy_class dummy_instance;

#endif // _DEPS_H_

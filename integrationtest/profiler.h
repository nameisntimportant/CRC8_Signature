#pragma once

#include <chrono>
#include <iostream>
#include <string>

namespace chr = std::chrono;

class LogDuration
{
public:
    explicit LogDuration(const std::string_view msg = "")
        : logMessage(msg)
        , startTimePoint(chr::steady_clock::now())
    {
    }

    ~LogDuration()
    {
        const auto duration = chr::steady_clock::now() - startTimePoint;
        std::cerr << logMessage << ": " << chr::duration_cast<chr::milliseconds>(duration).count()
                  << " ms" << std::endl;
    }

private:
    std::string logMessage;
    chr::steady_clock::time_point startTimePoint;
};

#define UNIQ_ID_IMPL(lineno) _a_local_var_##lineno
#define UNIQ_ID(lineno) UNIQ_ID_IMPL(lineno)

#define LOG_DURATION(message) LogDuration UNIQ_ID(__LINE__){message};

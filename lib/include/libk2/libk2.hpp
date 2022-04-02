#pragma once

extern "C" {
#include <unistd.h>
}

#include <string>

namespace k2 {

    std::string getVersion();

    std::string getActiveDevices();

    void registerTask(const std::string &device, const pid_t pid, std::int64_t interval_ns);

    void unregisterTask(const std::string &device, const pid_t pid);

    void unregisterAllTasks(const std::string &device);
}


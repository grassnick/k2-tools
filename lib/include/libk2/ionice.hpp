#pragma once

#include <string>

namespace ionice {

    enum class IoClass
    {
        None,
        RealTime,
        BestEffort,
        Idle,
        NA
    };

    enum class IoLevel
    {
        L0,
        L1,
        L2,
        L3,
        L4,
        L5,
        L6,
        L7,
        NA
    };


    [[nodiscard]] int ioPrioSet(const pid_t pid, const IoClass ioClass, const IoLevel ioLevel);

    [[nodiscard]] int ioPrioSet(const IoClass ioClass, const IoLevel ioLevel);

    [[nodiscard]] int ioPrioGet(const pid_t pid, IoClass &ioClass, IoLevel &ioLevel);

    [[nodiscard]] int ioPrioGet(IoClass &ioClass, IoLevel &ioLevel);

    [[nodiscard]] std::string toString(const IoClass ioClass);

    [[nodiscard]] std::string toString(const IoLevel ioLevel);

    int ioClassToInt(const ionice::IoClass ioClass);

    int ioLevelToInt(const ionice::IoLevel ioLevel);

    ionice::IoLevel ioLevelToEnum(const int cLevel);
}

std::ostream& operator<<(std::ostream& os, const ionice::IoClass ioClass);
std::ostream& operator<<(std::ostream& os, const ionice::IoLevel ioLevel);

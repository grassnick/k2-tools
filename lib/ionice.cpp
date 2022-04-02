#include "libk2/ionice.hpp"

extern "C" {
#include <unistd.h>
#include <linux/ioprio.h>
#include <sys/syscall.h>
}

// See https://www.kernel.org/doc/html/latest/block/ioprio.html

inline long ioPrioSetSyscall(int which, int who, int ioprio)
{
    return syscall(SYS_ioprio_set, which, who, ioprio);
}

inline long ioPrioGetSyscall(int which, int who)
{
    return syscall(SYS_ioprio_get, which, who);
}

std::string ionice::toString(const IoLevel ioLevel)
{
    switch (ioLevel) {
        case IoLevel::L0:
            return "0";
        case IoLevel::L1:
            return "1";
        case IoLevel::L2:
            return "2";
        case IoLevel::L3:
            return "3";
        case IoLevel::L4:
            return "4";
        case IoLevel::L5:
            return "5";
        case IoLevel::L6:
            return "6";
        case IoLevel::L7:
            return "7";
        default:
            return "N/A";
    }
}

ionice::IoClass ioClasstoEnum(const int cClass)
{
    switch (cClass) {
        case IOPRIO_CLASS_NONE:
            return ionice::IoClass::None;
        case IOPRIO_CLASS_RT:
            return ionice::IoClass::RealTime;
        case IOPRIO_CLASS_BE:
            return ionice::IoClass::BestEffort;
        case IOPRIO_CLASS_IDLE:
            return ionice::IoClass::Idle;
        default:
            return ionice::IoClass::NA;
    }
}

ionice::IoLevel ionice::ioLevelToEnum(const int cLevel)
{
    switch (cLevel) {
        case 0:
            return ionice::IoLevel::L0;
        case 1:
            return ionice::IoLevel::L1;
        case 2:
            return ionice::IoLevel::L2;
        case 3:
            return ionice::IoLevel::L3;
        case 4:
            return ionice::IoLevel::L4;
        case 5:
            return ionice::IoLevel::L5;
        case 6:
            return ionice::IoLevel::L6;
        case 7:
            return ionice::IoLevel::L7;
        default:
            return ionice::IoLevel::NA;
    }
}

int ionice::ioClassToInt(const ionice::IoClass ioClass)
{
    switch (ioClass) {
        case ionice::IoClass::None:
            return IOPRIO_CLASS_NONE;
        case ionice::IoClass::RealTime:
            return IOPRIO_CLASS_RT;
        case ionice::IoClass::BestEffort:
            return IOPRIO_CLASS_BE;
        case ionice::IoClass::Idle:
            return IOPRIO_CLASS_IDLE;
        default:
            return IOPRIO_CLASS_BE;
    }
}

int ionice::ioLevelToInt(const ionice::IoLevel ioLevel)
{
    switch (ioLevel) {
        case ionice::IoLevel::L0:
            return 0;
        case ionice::IoLevel::L1:
            return 1;
        case ionice::IoLevel::L2:
            return 2;
        case ionice::IoLevel::L3:
            return 3;
        case ionice::IoLevel::L4:
            return 4;
        case ionice::IoLevel::L5:
            return 5;
        case ionice::IoLevel::L6:
            return 6;
        case ionice::IoLevel::L7:
            return 7;
        default:
            return 4;
    }
}

int ionice::ioPrioSet(const pid_t pid, const IoClass ioClass, const IoLevel ioLevel)
{
    ioPrioSetSyscall(IOPRIO_WHO_PROCESS, pid, IOPRIO_PRIO_VALUE(ioClassToInt(ioClass), ioLevelToInt(ioLevel)));
    return errno;
}

int ionice::ioPrioSet(const IoClass ioClass, const IoLevel ioLevel)
{
    return ioPrioSet(getpid(), ioClass, ioLevel);
}

int ionice::ioPrioGet(const pid_t pid, IoClass &ioClass, IoLevel &ioLevel)
{
    const long ioPrio = ioPrioGetSyscall(IOPRIO_WHO_PROCESS, pid);
    if (errno) {
        return errno;
    }
    ioClass = ioClasstoEnum(IOPRIO_PRIO_CLASS(ioPrio));
    ioLevel = ioLevelToEnum(IOPRIO_PRIO_DATA(ioPrio));
    return 0;
}

int ionice::ioPrioGet(IoClass &ioClass, IoLevel &ioLevel)
{
    return ioPrioGet(getpid(), ioClass, ioLevel);
}

std::string ionice::toString(const IoClass ioClass)
{
    switch (ioClass) {
        case IoClass::None:
            return "none";
        case IoClass::RealTime:
            return "realtime";
        case IoClass::BestEffort:
            return "best-effort";
        case IoClass::Idle:
            return "idle";
        default:
            return "N/A";
    }
}

std::ostream& operator<<(std::ostream& os, ionice::IoClass ioClass){
    os << ionice::toString(ioClass);
    return os;
}

std::ostream& operator<<(std::ostream& os, ionice::IoLevel ioLevel) {
    os << ionice::toString(ioLevel);
    return os;
}
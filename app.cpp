extern "C" {
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
}

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <csignal>
#include <memory>


using namespace std::chrono_literals;

#include "k2.h"

namespace k2 {

    std::string k2IoschedDev()
    { return "/dev/k2-iosched"; }

    int openDriver(const std::string &devName, int &fd)
    {
        int result = open(devName.c_str(), O_RDWR);
        if (result < 0) {
            std::cerr << "Could not open " << devName << " : " << strerror(errno) << std::endl;
            return errno;
        }
        fd = result;
        return EXIT_SUCCESS;
    }

    int closeDriver(const int fd)
    {
        int result = close(fd);
        if (result < 0) {
            std::cerr << "Could not close file descriptor " << fd << " : " << strerror(errno) << std::endl;
            return errno;
        }
        return EXIT_SUCCESS;
    }

    void dynamicDeviceContext(const std::function<void(int &fd)> &func)
    {
        int fd = 0;
        int ret = openDriver(k2IoschedDev(), fd);
        if (ret) {
            return;
        }

        func(fd);

        closeDriver(fd);
    }

    void dynamicIoctlContext(const std::string &disk, const std::function<void(struct k2_ioctl &io)> &func)
    {
        struct k2_ioctl io{};
        memset(&io, 0, sizeof(io));

        char *dev = (char *) malloc(K2_IOCTL_BLK_DEV_NAME_LENGTH);
        strncpy(dev, disk.c_str(), K2_IOCTL_BLK_DEV_NAME_LENGTH);
        char *char_param = (char *) malloc(K2_IOCTL_CHAR_PARAM_LENGTH);
        io.string_param = char_param;
        io.blk_dev = dev;

        func(io);

        free(char_param);
        free(dev);
    }

    std::string getVersion()
    {
        int ret = 0;
        const std::string device;
        std::string version;

        dynamicDeviceContext([&](int &fd) {
            dynamicIoctlContext(device, [&](struct k2_ioctl &io) {
                ret = ioctl(fd, K2_IOC_GET_VERSION, &io);
                if (ret < 0) {
                    std::cerr << "ioctl could not determine version: " << strerror(errno)
                              << std::endl;
                } else {
                    version = io.string_param;
                }
            });
        });
        return version;
    }

    std::string getActiveDevices()
    {
        int ret = 0;
        const std::string device;
        std::string instances;

        dynamicDeviceContext([&](int &fd) {
            dynamicIoctlContext(device, [&](struct k2_ioctl &io) {
                ret = ioctl(fd, K2_IOC_GET_DEVICES, &io);
                if (ret < 0) {
                    std::cerr << "ioctl could not determine active devices: " << strerror(errno)
                              << std::endl;
                } else {
                    instances = io.string_param;
                }
            });
        });
        return instances;
    }

    void registerTask(const std::string &device, const pid_t pid, std::int64_t interval_ns)
    {
        int ret = 0;

        dynamicDeviceContext([&](int &fd) {
            dynamicIoctlContext(device, [&](struct k2_ioctl &io) {
                io.interval_ns = interval_ns;
                io.task_pid = pid;

                ret = ioctl(fd, K2_IOC_REGISTER_PERIODIC_TASK, &io);
                if (ret < 0) {
                    std::cerr << "ioctl register periodic task failed: " << strerror(errno)
                              << std::endl;
                } else {
                    std::cout << "Registered periodic task with pid " << io.task_pid
                              << " and interval time[ns] " << io.interval_ns << " for "
                              << io.blk_dev << std::endl;
                }
            });
        });

    }

    void unregisterTask(const std::string &device, const pid_t pid)
    {
        int ret = 0;
        dynamicDeviceContext([&](int &fd) {
            dynamicIoctlContext(device, [&](struct k2_ioctl &io) {
                io.task_pid = pid;

                ret = ioctl(fd, K2_IOC_UNREGISTER_PERIODIC_TASK, &io);
                if (ret < 0) {
                    std::cerr << "ioctl unregister periodic task failed: " << strerror(errno)
                              << std::endl;
                } else {
                    std::cout << "Unregistered periodic task with pid " << io.task_pid << " for " << io.blk_dev
                              << std::endl;
                }
            });
        });
    }

    void unregisterAllTasks(const std::string &device)
    {
        int ret = 0;
        dynamicDeviceContext([&](int &fd) {
            dynamicIoctlContext(device, [&](struct k2_ioctl &io) {

                ret = ioctl(fd, K2_IOC_UNREGISTER_ALL_PERIODIC_TASKS, &io);
                if (ret < 0) {
                    std::cerr << "ioctl unregister all periodic tasks failed: " << strerror(errno)
                              << std::endl;
                } else {
                    std::cout << "Unregistered all periodic tasks for " << io.blk_dev << std::endl;
                }
            });
        });
    }

}

// Global variables <3
const std::string disk("nvme0n1");
int fd = 0;
void *buffer = nullptr;
constexpr std::size_t NUM_BACKGROUND_PROCESSES = 8;
std::array<pid_t, NUM_BACKGROUND_PROCESSES> backgroundProcessPids;
bool childTerminate = false;


void terminate()
{
    std::cerr << "Process terminating gracefully" << std::endl;
    k2::unregisterAllTasks(disk);
    k2::closeDriver(fd);
    if (buffer != nullptr) {
        free(buffer);
    }
    exit(-1);
}

void mainSignalHandler(int signal)
{
    terminate();
}

void childSignalHandler(int signal)
{
    childTerminate = true;
}

void backgroundLoad(std::size_t index)
{
    std::signal(SIGINT, childSignalHandler);
    std::signal(SIGTERM, childSignalHandler);

    const std::size_t bs = 1024 << 5; // 1M
    void *childBuffer = malloc(bs);
    memset(childBuffer, 0, bs);

    // Set different prio levels to children
    setpriority(PRIO_PROCESS, 0, index);

    int f = open(("/dev/" + disk).c_str(), O_RDWR | O_SYNC);
    if (f < 0) {
        std::cout << "Could not open raw device file" << std::endl;
    } else {
        while (!childTerminate) {
            write(f, childBuffer, bs);
            std::this_thread::sleep_for(100ns);
        }
    }
    close(fd);
    free(childBuffer);
}


int main()
{
    for (std::size_t i = 0; i < NUM_BACKGROUND_PROCESSES; i++) {
        const pid_t forkPid = fork();
        if (forkPid < 0) {
            std::cerr << "Fork failed" << std::endl;
            terminate();
        } else if (forkPid == 0) {
            // Child process
            backgroundLoad(i);
            return 0;
        } else {
            // Parent process
            std::cout << "Starting background load on pid " << forkPid << std::endl;
            backgroundProcessPids[i] = forkPid;
        }
    }

    const auto mainPid = getpid();
    const auto interval = std::chrono::seconds(10);
    const auto intervalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count();

    std::signal(SIGINT, mainSignalHandler);
    std::signal(SIGTERM, mainSignalHandler);

    // Assign the highest possible priority to this process
    setpriority(PRIO_PROCESS, 0, PRIO_MAX);

    std::cout << "k2 version is " << k2::getVersion() << std::endl;
    std::cout << "k2 is active on " << k2::getActiveDevices() << std::endl;
    k2::registerTask(disk, mainPid, intervalNs);

    // Allocate memory for buffer to write
    const std::size_t bs = 1024 << 8; // 256K
    buffer = malloc(bs); // This mem
    memset(buffer, 0, bs);

    fd = open(("/dev/" + disk).c_str(), O_RDWR | O_SYNC);
    if (fd < 0) {
        std::cout << "Could not open raw device file" << std::endl;
    } else {

        for (std::size_t i = 0; i < 1024; i++) {
            write(fd, buffer, bs);
            std::this_thread::sleep_for(intervalNs * 1ns);
        }
    }
    std::this_thread::sleep_for(3s);

    // Close all background load processes
    for (const auto pid: backgroundProcessPids) {
        kill(pid, SIGTERM);
        int stat;
        waitpid(pid, &stat, 0);
    }

    k2::unregisterTask(disk, mainPid);
    k2::closeDriver(fd);
    free(buffer);

    return 0;
}

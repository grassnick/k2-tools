#include "libk2/libk2.hpp"

extern "C" {
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
}

#include <cerrno>
#include <cstring>
#include <iostream>
#include <functional>

#include <k2.h>


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

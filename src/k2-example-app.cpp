#include <array>
#include <iostream>
#include <csignal>
#include <cstring>
#include <thread>

using namespace std::chrono_literals;

extern "C" {
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/prctl.h>
}

#include "libk2/libk2.hpp"


// Global variables <3
const std::string disk("nvme0n1");
int fd = 0;
void *buffer = nullptr;
constexpr std::size_t NUM_BACKGROUND_PROCESSES = 0;
std::array<pid_t, NUM_BACKGROUND_PROCESSES> backgroundProcessPids;
bool childTerminate = false;


void terminate()
{
    std::cerr << "Process terminating gracefully" << std::endl;
    k2::unregisterAllTasks(disk);
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

    const std::size_t bs = 1024 << 12; // 4M
    std::string processName = "k2-app-" + std::to_string(index);
    prctl(PR_SET_NAME, processName.c_str());

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


int main(int argc, char **argv)
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
    std::size_t bs = 512;
    if (argc == 2) {
        char *tmp;
        bs = strtoul(argv[1], &tmp, 10) << 10;
    }
    buffer = malloc(bs);
    memset(buffer, 0, bs);

    fd = open(("/dev/" + disk).c_str(), O_RDWR | O_SYNC | O_APPEND);
    if (fd < 0) {
        std::cout << "Could not open raw device file" << std::endl;
    } else {

        for (std::size_t i = 0; i < 1024; i++) {
            write(fd, buffer, bs);
            //std::cout << "Issued Request of size " << bs << " - (" << (bs >> 10) << " KiByte)" <<  std::endl;
            //terminate();
            //std::this_thread::sleep_for(intervalNs * 1ns);
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
    free(buffer);

    return 0;
}

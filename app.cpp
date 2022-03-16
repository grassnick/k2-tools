extern "C" {
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
}

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include "k2.h"

static std::string k2_iosched_dev() { return "/dev/k2-iosched"; }

int open_driver(const char *dev) {

  int f = open(dev, O_RDWR);
  if (f < 0) {
    printf("ERROR: could not open \"%s\".\n", dev);
    printf("    errno = %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  return f;
}

void close_driver(int fd_driver) {

  int result = close(fd_driver);
  if (result == -1) {
    printf("    errno = %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

int main() {
  int error = 0;
  const char *dev = "/dev/nvme0n1";
  struct k2_ioctl io {};
  char *char_param = (char *)malloc(K2_IOCTL_CHAR_PARAM_LENGTH);

  auto fd = open_driver(k2_iosched_dev().c_str());

  memset(&io, 0, sizeof(io));
  io.string_param = char_param;
  error = ioctl(fd, K2_IOC_GET_VERSION, &io);
  if (error < 0) {
    std::cerr << "ioctl test none failed: " << strerror(errno) << std::endl;
    goto finally;
  }

  std::cout << "Version is: " << io.string_param << std::endl;

#if true
  memset(&io, 0, sizeof(struct k2_ioctl));
  io.blk_dev = dev;
  error = ioctl(fd, K2_IOC_CURRENT_INFLIGHT_LATENCY, &io);
  if (error < 0) {
    std::cerr << "ioctl wr test failed: " << strerror(errno) << std::endl;
    goto finally;
  }

  std::cout << "Device " << io.blk_dev << " has current inflight of "
            << io.u32_param << std::endl;
#endif

#if true
  memset(&io, 0, sizeof(struct k2_ioctl));
  io.blk_dev = dev;
  std::cout << io.blk_dev << " " << strlen(io.blk_dev) << std::endl;

  io.interval_ns = 5000;
  io.task_pid = 8000;

  error = ioctl(fd, K2_IOC_REGISTER_PERIODIC_TASK, &io);
  if (error < 0) {
    std::cerr << "ioctl register periodic task failed: " << strerror(errno)
              << std::endl;
    goto finally;
  }

  std::cout << "Registered periodic task with pid " << io.task_pid
            << " and interval time[ns] " << io.interval_ns << "for "
            << io.blk_dev << std::endl;
#endif

#if true
  memset(&io, 0, sizeof(struct k2_ioctl));
  io.string_param = char_param;
  error = ioctl(fd, K2_IOC_GET_DEVICES, &io);
  if (error < 0) {
    std::cerr << "ioctl register periodic task failed: " << strerror(errno)
              << std::endl;
    goto finally;
  }

  std::cout << "Running instances of k2: " << io.string_param << std::endl;
#endif

finally:
  free(char_param);
  close_driver(fd);
  return error;
}

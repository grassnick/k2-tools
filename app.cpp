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
  std::uint32_t val = 0;
  int error = 0;

  const char *dev = "/dev/nvme0n1";
  __u32 read = 0;
  // For some reason, a stack allocated memory region would not work here, so just allocate it on heap ...
  int *writeP = (int *)malloc(sizeof(int));
  *writeP = 42;
  struct k2_ioctl io {};

  auto fd = open_driver(k2_iosched_dev().c_str());
  error = ioctl(fd, K2_IOC_GET_VERSION, &val);
  if (error < 0) {
    std::cerr << "ioctl test none failed: " << strerror(errno) << std::endl;
    goto finally;
  }

  std::cout << "Value is: " << val << std::endl;

#if true
  error = ioctl(fd, K2_IOC_READ_TEST, &read);
  if (error < 0) {
    std::cerr << "ioctl read test failed " << strerror(errno) << std::endl;
    goto finally;
  }
  std::cout << "Read Test: " << read << std::endl;
#endif

#if true
  error = ioctl(fd, K2_IOC_WRITE_TEST, writeP);
  if (error < 0) {
    std::cerr << "ioctl write test failed: " << strerror(errno) <<  std::endl;
    goto finally;
  }
  std::cout << "Write Test: " << *writeP << std::endl;
#endif

#if true
  error = ioctl(fd, K2_IOC_WRITE_READ_TEST, writeP);
  if (error < 0) {
    std::cerr << "ioctl write read test failed: " << strerror(errno) <<  std::endl;
    goto finally;
  }
  std::cout << "Write Read Test: " << *writeP << std::endl;
#endif

#if true

  memset(&io, 0, sizeof(struct k2_ioctl));

  io.dev_name = dev;
  io.dev_name_len = strlen(io.dev_name);

  error = ioctl(fd, K2_IOC_CURRENT_INFLIGHT_LATENCY, &io);
  if (error < 0) {
    std::cerr << "ioctl wr test failed: " << strerror(errno) << std::endl;
    goto finally;
  }

  std::cout << "Device " << io.dev_name << " has current inflight of "
            << io.u32_ret << std::endl;
#endif

finally:
  free(writeP);
  close_driver(fd);
  return error;
}
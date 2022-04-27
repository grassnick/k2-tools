#include <iostream>

/**
 * @see kdev_t.h in the kernel headers
 */
#define MINORBITS	20
#define MINORMASK	((1U << MINORBITS) - 1)

#define MAJOR(dev)	((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)	((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi)	(((ma) << MINORBITS) | (mi))


/**
 * @brief Small helper tool that translates major:minor device numbers to the internal u32 representation in the kernel
 * @details Used to filter requests captured by lttng for the device that is benchmarked and limits trace file size
 */
int main(int argc, char** argv)
{
  if (argc != 2) {
    std::cerr << "Expected 1 parameter, received " << argc - 1 << std::endl;
    return 1;
  }

  std::string dev_t = argv[1];

  std::string majorString;
  std::string minorString;

  bool finishedMajor = false;
  for (const auto& it : dev_t) {
    if (it == ':') {
      finishedMajor = true;
    } else {
      if (finishedMajor) {
        minorString += it;
      } else {
        majorString += it;
      }
    }
  }

  std::uint32_t major = std::stoul(majorString);
  std::uint32_t minor = std::stoul(minorString);

  std::cout << MKDEV(major, minor) << std::endl;

  return 0;
}
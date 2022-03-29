#include "libk2/libk2.hpp"

#include <argparse/argparse.hpp>

enum class OperationMode
{
    Register,
    Unregister,
    UnregisterAll,
    NotSupported
};

class K2App
{
protected:
    const std::string device;
    const std::optional<pid_t> pid;
    const std::optional<std::int64_t> interval;
    const OperationMode mode;

public:
    K2App() = delete;

    K2App(const K2App &other) = delete;


    K2App(const std::string &device, const std::optional<pid_t> &pid, const std::optional<std::int64_t> &interval,
          const OperationMode mode) :
            device(device), pid(pid), interval(interval), mode(mode)
    {}

    virtual K2App operator=(const K2App &other) = delete;

    virtual int run()
    {
        int ret = 0;
        switch (this->mode) {
            case OperationMode::Register:
                if (!pid) {
                    std::cerr << "pid is required" << std::endl;
                    return 1;
                }
                if (!this->interval) {
                    std::cerr << "interval is required" << std::endl;
                    return 1;
                }
                k2::registerTask(this->device, *this->pid, *this->interval);
                break;
            case OperationMode::Unregister:
                if (!pid) {
                    std::cerr << "pid is required" << std::endl;
                    return 1;
                }
                k2::unregisterTask(device, *this->pid);
                break;
            case OperationMode::UnregisterAll:
                k2::unregisterAllTasks(device);
                break;
            default:
                ret = 1;
                break;
        }
        return ret;
    }
};

int main(int argc, char **argv)
{

    argparse::ArgumentParser program("k2-register-task", "0.1");

    program.add_argument("--mode", "-m")
            .required()
            .nargs(1)
            .action([](const std::string &value) {
                static const std::vector<std::string> choices = {"reg", "unreg", "unreg_all"};
                if (std::find(choices.begin(), choices.end(), value) != choices.end()) {
                    return value;
                }
                return std::string{"reg"};
            });

    program.add_argument("--device", "-d")
            .required()
            .help("set the device to perform the operation on");

    program.add_argument("--pid", "-p")
            .scan<'i', pid_t>()
            .help("set the pid of the process to register");

    program.add_argument("--interval", "-i")
            .scan<'i', std::int64_t>()
            .help("set the interval in ns of the process to register");


    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }

    const auto op = program.get<std::string>("--mode");
    OperationMode mode;

    if (op.compare("reg") == 0) {
        mode = OperationMode::Register;
    } else if (op.compare("unreg") == 0) {
        mode = OperationMode::Unregister;
    } else if (op.compare("unreg_all") == 0) {
        mode = OperationMode::UnregisterAll;
    } else {
        mode = OperationMode::NotSupported;
    }

    auto device = program.get<std::string>("--device");
    auto pid = program.get<pid_t>("--pid");
    auto interval = program.present<std::int64_t>("--interval");

    K2App app(device, pid, interval, mode);
    return app.run();
}

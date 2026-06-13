#ifndef SEMON_CORE_SHELL_HPP
#define SEMON_CORE_SHELL_HPP

namespace semon {
namespace core {

class Shell {
public:
    Shell();

    int run(int argc, char** argv);

private:
    bool running_;
};

} // namespace core
} // namespace semon

#endif

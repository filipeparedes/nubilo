module;
#include <string>

export module version;

export namespace nubilo {
    std::string getVersion() {
        return "0.0.1-dev";
    }
}
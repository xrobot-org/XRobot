#pragma once
#include <vector>
#include "module.hpp"

namespace xrobot {

class Application {
public:
    void RegisterModule(Module* m) {
        modules_.push_back(m);
    }

    void Init() {
        for (auto* m : modules_) {
            m->OnInit();
        }
    }

    void MainLoop() {
        for (int i = 0; i < 3; ++i) {
            for (auto* m : modules_) {
                m->OnTick();
            }
        }
    }

    void Shutdown() {
        for (auto* m : modules_) {
            m->OnShutdown();
        }
    }

private:
    std::vector<Module*> modules_;
};

} // namespace xrobot
#pragma once

namespace xrobot {

class Module {
public:
    virtual void OnInit() {}
    virtual void OnTick() {}
    virtual void OnShutdown() {}
    virtual ~Module() = default;
};

} // namespace xrobot
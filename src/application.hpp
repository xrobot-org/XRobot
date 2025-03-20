#pragma once
#include "list.hpp"
#include "peripheral_manager.hpp"

namespace XRobot {

class Application {
public:
  virtual void OnInit(PeripheralManager &pm) = 0;
  virtual void OnMonitor() = 0;
  virtual ~Application() = default;
};

class ApplicationManager {
public:
  LibXR::List app_list_;

  void RegisterApplication(Application &app) {
    auto node = new LibXR::List::Node<Application *>(&app);
    app_list_.Add(*node);
  }

  void InitAll(PeripheralManager &pm) {
    app_list_.Foreach<Application *>([&](Application *app) {
      app->OnInit(pm);
      return ErrorCode::OK;
    });
  }

  void MonitorAll() {
    app_list_.Foreach<Application *>([](Application *app) {
      app->OnMonitor();
      return ErrorCode::OK;
    });
  }

  size_t Size() { return app_list_.Size(); }
};

} // namespace XRobot
#include <string.h>

namespace Device {
class Motor {
 public:
  typedef struct {
    float rotor_abs_angle;  /* 转子绝对角度 单位：rad */
    float rotational_speed; /* 转速 单位：rpm */
    float torque_current;   /* 转矩电流 单位：A*/
    float temp;             /* 电机温度 单位：℃*/
  } Feedback;

  Motor(const char *name) {
    strncpy(this->name_, name, sizeof(this->name_));
    memset(&(this->feedback_), 0, sizeof(this->feedback_));
  }

  virtual void Control(float output) = 0;

  virtual bool Update() = 0;

  virtual void Relax() = 0;

  char name_[20];

  Feedback feedback_;

  bool reverse_;
};
}  // namespace Device

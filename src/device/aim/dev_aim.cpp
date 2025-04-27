#include "dev_aim.hpp"
using namespace Device;

#define AI_LEN_RX_BUFF (sizeof(AIM::RefForAI_t))
#define AI_LEN_TX_BUFF (sizeof(AIM::TranToAI_t))

static uint8_t rxbuf[AI_LEN_RX_BUFF];
static uint8_t txbuf[AI_LEN_TX_BUFF];

/* clang-format off */
AIM::AIM()
    : event_(Message::Event::FindEvent("cmd_event")),
      cmd_tp_("cmd_ai"),
      data_ready_(false)
{

  auto rx_cplt_callback = [](void *arg) {
    AIM *aim = static_cast<AIM *>(arg);
    aim->data_ready_.Post();
  };

  bsp_uart_register_callback(BSP_UART_AI, BSP_UART_RX_CPLT_CB, rx_cplt_callback,
                             this);

  Component::CMD::RegisterController(this->cmd_tp_);
 auto ai_thread = [](AIM *aim) {

    auto eulr_sub = Message::Subscriber<Component::Type::Eulr>("imu_eulr");

     while (1)
     {
        /* 接收imu和裁判系统数据 */
        eulr_sub.DumpData(aim->eulr_);

        /* 接收上位机数据 */
        aim->StartRecv();
        if (aim->data_ready_.Wait(0))
        {
            aim->PraseHost();
            aim->PackCMD();
        }

        //发送数据给上位机
        aim->PackMCU();
        aim->StartTran();

        System::Thread::Sleep(2);
     }
    };
    this->thread_.Create(ai_thread, this, "aim_thread", DEVICE_AI_TASK_STACK_DEPTH,
                       System::Thread::REALTIME);
}

double convert_to_0_to_2pi(double theta_prime) {
  double theta = fmod(theta_prime + 2 * M_PI, 2 * M_PI);
  return theta;
}

bool AIM::PackCMD() {
  /* 确保遥控器开关最高控制权，关遥控器即断控 */
  if (!Component::CMD::Online()) {
    return false;
  }

    if(from_host_.yaw == 0 && from_host_.pitch == 0)
    {
      this->to_cmd_.yaw = this->eulr_.yaw;
      this->to_cmd_.pitch = this->eulr_.pit;
      this->to_cmd_.roll = this->eulr_.rol;
      return 0;
    }

  /* 控制源：AI */
  if (Component::CMD::GetCtrlSource() == Component::CMD::CTRL_SOURCE_AI) {
    /* OP控制模式，用于鼠标右键自瞄 */
    if (Component::CMD::GetCtrlMode() == Component::CMD::CMD_OP_CTRL) {

  if(from_host_.header == 0xA5)
  {
    this->to_cmd_.yaw = from_host_.yaw;
    this->to_cmd_.pitch = from_host_.pitch;
    this->to_cmd_.roll = this->eulr_.rol;
  }

      memcpy(&(this->cmd_.gimbal.eulr), &(this->to_cmd_),
             sizeof(this->cmd_.gimbal.eulr));

      this->cmd_.ctrl_source = Component::CMD::CTRL_SOURCE_AI;

      this->cmd_tp_.Publish(this->cmd_);
      this->cmd_tp_.Publish(this->cmd_);
      this->cmd_tp_.Publish(this->cmd_);
    }
  }
  return true;
}

bool AIM::PraseHost() {
   if (Component::CRC16::Verify(rxbuf, sizeof(this->from_host_))) {
    this->cmd_.online = true;
    this->last_online_time_ = bsp_time_get_ms();
    memcpy(&(this->from_host_), rxbuf, sizeof(this->from_host_));
    memset(rxbuf, 0, AI_LEN_RX_BUFF);
    return true;
  }
  return false;
}

double convert_to_minus_pi_to_pi(double theta) {
  while (theta > M_PI) {
    theta -= 2 * M_PI;
  }
  while (theta < -M_PI) {
    theta += 2 * M_PI;
  }
  return theta;
}

bool AIM::PackMCU()
{
    this->to_host_.header = 0x5A;
    this->to_host_.detect_color = 0;
    this->to_host_.reset_tracker = 0;
    this->to_host_.current_v = 24;
    this->to_host_.yaw = this->eulr_.yaw;
    this->to_host_.pitch = this->eulr_.pit;
    this->to_host_.roll = this->eulr_.rol;
    this->to_host_.aim_x = this->from_host_.x;
    this->to_host_.aim_y = this->from_host_.y;
    this->to_host_.aim_z = this->from_host_.z;
    this->to_host_.checksum = Component::CRC16::Calculate(
      reinterpret_cast<const uint8_t *>(&(this->to_host_)),
      sizeof(this->to_host_) - sizeof(uint16_t), CRC16_INIT);

    return true;
}
bool AIM::StartTran()
{
    size_t len = sizeof(this->to_host_);

    void *src = NULL;
    src = &(this->to_host_);
    memcpy(txbuf, src, len);
    return bsp_uart_transmit(BSP_UART_AI, txbuf, len,false) == BSP_OK;
}

bool AIM::StartRecv()
{
  return bsp_uart_receive(BSP_UART_AI, rxbuf, sizeof(rxbuf), false) == BSP_OK;
}

/*
  ���ݿ�������
*/

/* Includes ----------------------------------------------------------------- */
#include "component\limiter.h"
#include "device\can.h"
#include "device\referee.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
#define CHASSIS_POWER_MAX_WITHOUT_REF 80.0f
#define CAP_STATUS_THRESHOLD_CHARGE 10.0f
#define CAP_STATUS_THRESHOLD_RUNNING1 16.0f
#define CAP_STATUS_THRESHOLD_RUNNING2 20.0f
#define CAP_STATUS_THRESHOLD_RUNNING3 24.0f

/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */

#ifdef DEBUG
CAN_t *can_cap;  //这个名字也不合适
CAN_RawRx_t can_cap_rx;
Referee_t *referee;
#else
static CAN_t can;
CAN_RawRx_t can_motor_rx;
Referee_t *ref;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void Task_Cap(void *argument) {
  (void)argument;

  /* Device Setup */
  // TODO: 这样肯定不行，考虑把cap放到motor.c中，motor改为can
  while ((can_cap = CAN_GetDevice()) ==
         NULL)  // TODO: 不要用这种共享指针，又回去了，用msgq
    ;
  can_cap->cap_status =
      CAP_STATUS_OFFLINE;  // TODO: 这种用宏定义，#define
                           // CAN_CapUpdateStatus(can, CAP_STATUS_CHARGING)
  while (osMessageQueueGet(can_cap->msgq_raw_motor, &can_cap_rx, 0, 5) !=
         osOK) {
  } /* �ȴ��������� */
  can_cap->cap_status = CAP_STATUS_CHARGING;
  referee = Referee_GetDevice();  // TODO: 不要用这种共享指针，又回去了，用msgq
  /* Task Setup */
  while (1) {
    if (osMessageQueueGet(can_cap->msgq_raw_motor, &can_cap_rx, 0, 5) == osOK)
      CAN_Cap_Decode(&(can_cap->cap_feedback), can_cap_rx.rx_data);

    /* �����빦�ʽ������� */
    if (referee->ref_status != REF_STATUS_RUNNING) {
      /* ����ϵͳδִ�� */  // TODO: 还是用msgq好，收不到就是没上线，就这么简单
      CAN_CapControl(
          CHASSIS_POWER_MAX_WITHOUT_REF);  // TODO:
                                           // 这是不是没有频率控制，命令以多少的速度往外发的？
    } else { /* ����ϵͳ������ */
      CAN_CapControl(
          PowerLimit_CapInput(referee->power_heat.chassis_watt,
                              referee->robot_status.chassis_power_limit,
                              referee->power_heat.chassis_pwr_buff));
    }

    /* ���ݵ��ݵ�ѹ�޸Ĺ���״̬ */
    float cap_volt = can_cap->cap_feedback.cap_volt;
    if (cap_volt < CAP_STATUS_THRESHOLD_CHARGE)
      can_cap->cap_status = CAP_STATUS_CHARGING;
    else if (cap_volt < CAP_STATUS_THRESHOLD_RUNNING1)
      can_cap->cap_status =
          CAP_STATUS_RUNNING1;  // TODO: RUNNING123什么区别？
                                // component/capacity里面有个计算剩余电量的函数，用那个
    else if (cap_volt < CAP_STATUS_THRESHOLD_RUNNING2)
      can_cap->cap_status = CAP_STATUS_RUNNING2;
    else if (cap_volt < CAP_STATUS_THRESHOLD_RUNNING3)
      can_cap->cap_status = CAP_STATUS_RUNNING3;
  }
}

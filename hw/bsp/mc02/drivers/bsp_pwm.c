#include "bsp_pwm.h"

#include "main.h"

#define WS2812_LowLevel 0xC0
#define WS2812_HighLevel 0xF0

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim7;
extern TIM_HandleTypeDef htim12;
extern SPI_HandleTypeDef hspi6;

static uint8_t ws2812_color_grb[3] = {0}, spi_tx_buff[24] = {0};

typedef struct {
  TIM_HandleTypeDef *tim;
  uint16_t channel;
} bsp_pwm_config_t;

static void ws2812_ctrl() {
  for (int i = 0; i < 8; i++) {
    spi_tx_buff[7 - i] =
        (((ws2812_color_grb[0] >> i) & 0x01) ? WS2812_HighLevel
                                             : WS2812_LowLevel) >>
        1;
    spi_tx_buff[15 - i] =
        (((ws2812_color_grb[1] >> i) & 0x01) ? WS2812_HighLevel
                                             : WS2812_LowLevel) >>
        1;
    spi_tx_buff[23 - i] =
        (((ws2812_color_grb[2] >> i) & 0x01) ? WS2812_HighLevel
                                             : WS2812_LowLevel) >>
        1;
  }
  HAL_SPI_Transmit_DMA(&hspi6, spi_tx_buff, sizeof(spi_tx_buff));
}

static bsp_pwm_config_t bsp_pwm_map[BSP_PWM_NUMBER] = {
    [BSP_PWM_IMU_HEAT] = {&htim3, TIM_CHANNEL_4},
    [BSP_PWM_BUZZER] = {&htim12, TIM_CHANNEL_2},
    [BSP_PWM_LED_GRN] = {&htim1, TIM_CHANNEL_2},
    [BSP_PWM_LED_RED] = {&htim1, TIM_CHANNEL_2},
    [BSP_PWM_LED_BLU] = {&htim1, TIM_CHANNEL_2},
    [BSP_PWM_SERVO_A] = {&htim1, TIM_CHANNEL_1},
    [BSP_PWM_SERVO_B] = {&htim1, TIM_CHANNEL_3},
    [BSP_PWM_SERVO_C] = {&htim2, TIM_CHANNEL_1},
    [BSP_PWM_SERVO_D] = {&htim2, TIM_CHANNEL_3},
};

bsp_status_t bsp_pwm_start(bsp_pwm_channel_t ch) {
  if (ch >= BSP_PWM_LED_GRN && ch <= BSP_PWM_LED_BLU) {
    ws2812_ctrl();
    return BSP_OK;
  }
  HAL_TIM_PWM_Start(bsp_pwm_map[ch].tim, bsp_pwm_map[ch].channel);

  return BSP_OK;
}

bsp_status_t bsp_pwm_set_comp(bsp_pwm_channel_t ch, float duty_cycle) {
  if (duty_cycle > 1.0f) {
    return BSP_ERR;
  }

  if (duty_cycle < 0.0f) {
    duty_cycle = 0.f;
  }

  if (ch >= BSP_PWM_LED_GRN && ch <= BSP_PWM_LED_BLU) {
    ws2812_color_grb[ch - BSP_PWM_LED_GRN] = (uint8_t)(duty_cycle * 255.0f);
    ws2812_ctrl();
    return BSP_OK;
  }

  /* 通过PWM通道对应定时器重载值和给定占空比，计算PWM周期值 */
  uint16_t pulse = (uint16_t)(duty_cycle * (float)__HAL_TIM_GET_AUTORELOAD(
                                               bsp_pwm_map[ch].tim));

  __HAL_TIM_SET_COMPARE(bsp_pwm_map[ch].tim, bsp_pwm_map[ch].channel, pulse);

  return BSP_OK;
}

bsp_status_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq) {
  uint16_t reload = (uint16_t)(1000000U / freq);

  if (ch >= BSP_PWM_LED_GRN && ch <= BSP_PWM_LED_BLU) {
    return BSP_OK;
  }

  if (reload > 0) {
    __HAL_TIM_PRESCALER(bsp_pwm_map[ch].tim, reload);
  } else {
    return BSP_ERR;
  }

  return BSP_OK;
}

bsp_status_t bsp_pwm_stop(bsp_pwm_channel_t ch) {
  HAL_TIM_PWM_Stop(bsp_pwm_map[ch].tim, bsp_pwm_map[ch].channel);
  return BSP_OK;
}

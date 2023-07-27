#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "bsp.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

typedef enum {
  UUID_GATTS_SERVICE_TEST = 0x00FF,
  UUID_GATTS_CHAR_TEST_A = 0xFF01,
  UUID_GATTS_CHAR_TEST_B = 0xFF02,
  UUID_GATTS_CHAR_TEST_C = 0xFF03,
  UUID_PRIMARY_SERVICE = ESP_GATT_UUID_PRI_SERVICE,
  UUID_CHARACTER_DECL = ESP_GATT_UUID_CHAR_DECLARE,
  UUID_CLIENT_CONFIG = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,
} bsp_ble_gatt_uuid_t;

typedef struct {
  bsp_ble_gatt_uuid_t val_uuid;
  uint8_t char_value[10];
} bsp_ble_charater_t;

/* Attributes State Machine */
typedef enum {
  IDX_SVC,
  IDX_CHAR_VOL, /* 电压 append Notify */
  IDX_CHAR_VAL_VOL,
  IDX_CHAR_CFG_VOL,

  IDX_CHAR_B, /* TestB Noappend Notify */
  IDX_CHAR_VAL_B,

  IDX_CHAR_C,
  IDX_CHAR_VAL_C,

  HRS_IDX_NB,
} bsp_ble_ID_t;

typedef struct {
  uint8_t *prepare_buf;
  bsp_status_t prepare_len;
} bsp_prepare_env_t;

typedef struct {
  uint8_t adv_config_done;
  uint8_t heart_measurement_ccc[2];
  uint16_t heart_rate_handle_table[HRS_IDX_NB];
  esp_ble_adv_params_t adv_params;
  esp_gatts_attr_db_t gatt_db[HRS_IDX_NB];
  bsp_prepare_env_t write_env;
  bsp_ble_charater_t char_inst[3];
} bsp_ble_config_t;

typedef struct {
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_handle;
  esp_gatt_srvc_id_t service_id;
  uint16_t char_handle;
  esp_bt_uuid_t char_uuid;
  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  esp_bt_uuid_t descr_uuid;
} bsp_gatts_profile_t;

void bsp_ble_init();

#ifdef __cplusplus
}
#endif

#include "bsp_ble.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"

#define PROFILE_NUM 1
#define PROFILE_APP_IDX 0
#define ESP_APP_ID 0x55
#define SAMPLE_DEVICE_NAME "RM_ESP32"
#define SVC_INST_ID 0

#define GATTS_CHAR_VAL_LEN_MAX 500
#define PREPARE_BUF_MAX_SIZE 1024
#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))

#define ADV_CONFIG_FLAG (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1)

/* 广播包定义 */
static uint8_t raw_adv_data[] = {
    /* flags */
    0x02, 0x01, 0x06,
    /* tx power*/
    0x02, 0x0a, 0xeb,
    /* service uuid */
    0x03, 0x03, 0xFF, 0x00,
    /* device name */
    0x0a, 0x09, 'R', 'O', 'B', '_', 'E', 'S', 'P', '3', '2'};
static uint8_t raw_scan_rsp_data[] = {
    /* flags */
    0x02, 0x01, 0x06,
    /* tx power */
    0x02, 0x0a, 0xeb,
    /* service uuid */
    0x03, 0x03, 0xFF, 0x00};
/* 服务器 UUID */
static const bsp_ble_gatt_uuid_t gatts_service_uuid = UUID_GATTS_SERVICE_TEST;
static const uint16_t primary_service_uuid = UUID_PRIMARY_SERVICE;
static const uint16_t character_declaration_uuid = UUID_CHARACTER_DECL;
static const uint16_t character_client_config_uuid = UUID_CLIENT_CONFIG;
/* character read and write */
static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write_notify =
    ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ |
    ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static bsp_ble_config_t config = {
    .adv_params =
        {
            .adv_int_min = 0x20,
            .adv_int_max = 0x40,
            .adv_type = ADV_TYPE_IND,
            .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
            .channel_map = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        },
    .char_inst =
        {
            {.val_uuid = UUID_GATTS_CHAR_TEST_A, .char_value = {"RM ESPA"}},
            {.val_uuid = UUID_GATTS_CHAR_TEST_B, .char_value = {"RM ESPB"}},
            {.val_uuid = UUID_GATTS_CHAR_TEST_C, .char_value = {"RM ESPC"}},
        },
    .heart_measurement_ccc = {0x00, 0x00},
    .gatt_db =
        {
            // Service Declaration
            [IDX_SVC] = {{ESP_GATT_AUTO_RSP},
                         {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid,
                          ESP_GATT_PERM_READ, sizeof(uint16_t),
                          sizeof(gatts_service_uuid),
                          (uint8_t *)&gatts_service_uuid}},

            /* Characteristic Declaration A:0x2902 */
            [IDX_CHAR_VOL] = {{ESP_GATT_AUTO_RSP},
                              {ESP_UUID_LEN_16,
                               (uint8_t *)&character_declaration_uuid,
                               ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE,
                               CHAR_DECLARATION_SIZE,
                               (uint8_t *)&char_prop_read_write_notify}},

            /* Characteristic Value */
            [IDX_CHAR_VAL_VOL] = {{ESP_GATT_AUTO_RSP},
                                  {ESP_UUID_LEN_16,
                                   (uint8_t *)&config.char_inst[0].val_uuid,
                                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                   GATTS_CHAR_VAL_LEN_MAX,
                                   sizeof(config.char_inst[0].char_value),
                                   (uint8_t *)config.char_inst[0].char_value}},

            /* Client Characteristic Configuration Descriptor */
            [IDX_CHAR_CFG_VOL] = {{ESP_GATT_AUTO_RSP},
                                  {ESP_UUID_LEN_16,
                                   (uint8_t *)&character_client_config_uuid,
                                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                   sizeof(uint16_t),
                                   sizeof(config.heart_measurement_ccc),
                                   (uint8_t *)config.heart_measurement_ccc}},

            /* Characteristic Declaration */
            [IDX_CHAR_B] = {{ESP_GATT_AUTO_RSP},
                            {ESP_UUID_LEN_16,
                             (uint8_t *)&character_declaration_uuid,
                             ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE,
                             CHAR_DECLARATION_SIZE,
                             (uint8_t *)&char_prop_read}},

            /* Characteristic Value */
            [IDX_CHAR_VAL_B] = {{ESP_GATT_AUTO_RSP},
                                {ESP_UUID_LEN_16,
                                 (uint8_t *)&config.char_inst[1].val_uuid,
                                 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                 GATTS_CHAR_VAL_LEN_MAX,
                                 sizeof(config.char_inst[1].char_value),
                                 (uint8_t *)config.char_inst[1].char_value}},

            /* Characteristic Declaration */
            [IDX_CHAR_C] = {{ESP_GATT_AUTO_RSP},
                            {ESP_UUID_LEN_16,
                             (uint8_t *)&character_declaration_uuid,
                             ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE,
                             CHAR_DECLARATION_SIZE,
                             (uint8_t *)&char_prop_write}},

            /* Characteristic Value */
            [IDX_CHAR_VAL_C] = {{ESP_GATT_AUTO_RSP},
                                {ESP_UUID_LEN_16,
                                 (uint8_t *)&config.char_inst[2].val_uuid,
                                 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                 GATTS_CHAR_VAL_LEN_MAX,
                                 sizeof(config.char_inst[2].char_value),
                                 (uint8_t *)config.char_inst[2].char_value}},

        },
};
/* Full Database Description - Used to add attributes into the database */
static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the
 * gatts_if returned by ESP_GATTS_REG_EVT */
static bsp_gatts_profile_t heart_rate_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] =
        {
            .gatts_cb = gatts_profile_event_handler,
            .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
            ESP_GATT_IF_NONE */
        },
};
static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param) {
  switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
      config.adv_config_done &= (~ADV_CONFIG_FLAG);
      if (config.adv_config_done == 0)
        esp_ble_gap_start_advertising(&config.adv_params);
      break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
      config.adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
      if (config.adv_config_done == 0)
        esp_ble_gap_start_advertising(&config.adv_params);
      break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      /* advertising start complete event to indicate advertising start
       * successfully or failed */
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
    default:
      break;
  }
}

static void example_prepare_write_event_env(
    esp_gatt_if_t gatts_if, bsp_prepare_env_t *prepare_write_env,
    esp_ble_gatts_cb_param_t *param) {
  esp_gatt_status_t status = ESP_GATT_OK;
  if (prepare_write_env->prepare_buf == NULL) {
    prepare_write_env->prepare_buf =
        (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
    prepare_write_env->prepare_len = 0;
    if (prepare_write_env->prepare_buf == NULL) {
      status = ESP_GATT_NO_RESOURCES;
    }
  } else {
    if (param->write.offset > PREPARE_BUF_MAX_SIZE) {
      status = ESP_GATT_INVALID_OFFSET;
    } else if ((param->write.offset + param->write.len) >
               PREPARE_BUF_MAX_SIZE) {
      status = ESP_GATT_INVALID_ATTR_LEN;
    }
  }
  /*send response when param->write.need_rsp is true */
  if (param->write.need_rsp) {
    esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
    if (gatt_rsp != NULL) {
      gatt_rsp->attr_value.len = param->write.len;
      gatt_rsp->attr_value.handle = param->write.handle;
      gatt_rsp->attr_value.offset = param->write.offset;
      gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
      memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
      esp_err_t response_err =
          esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                      param->write.trans_id, status, gatt_rsp);
      free(gatt_rsp);
    }
  }
  if (status != ESP_GATT_OK) {
    return;
  }
  memcpy(prepare_write_env->prepare_buf + param->write.offset,
         param->write.value, param->write.len);
  prepare_write_env->prepare_len += param->write.len;
}

static void example_exec_write_event_env(bsp_prepare_env_t *prepare_write_env,
                                         esp_ble_gatts_cb_param_t *param) {
  if (prepare_write_env->prepare_buf) {
    free(prepare_write_env->prepare_buf);
    prepare_write_env->prepare_buf = NULL;
  }
  prepare_write_env->prepare_len = 0;
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param) {
  switch (event) {
    case ESP_GATTS_REG_EVT: {
      /* Device ID */
      esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
      /* 广播 */
      esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
      config.adv_config_done |= ADV_CONFIG_FLAG;
      esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data,
                                           sizeof(raw_scan_rsp_data));
      config.adv_config_done |= SCAN_RSP_CONFIG_FLAG;
      esp_ble_gatts_create_attr_tab(config.gatt_db, gatts_if, HRS_IDX_NB,
                                    SVC_INST_ID);
    } break;
    case ESP_GATTS_READ_EVT: {
      /* 读ESP */
      uint8_t sensor[] = {0xFE, rand(), 0xAF};
      if (config.heart_rate_handle_table[IDX_CHAR_VAL_VOL] ==
          param->read.handle) {
        esp_ble_gatts_set_attr_value(param->read.handle,
                                     sizeof(config.char_inst[0].char_value),
                                     (uint8_t *)config.char_inst[0].char_value);
      }
    } break;
    case ESP_GATTS_WRITE_EVT: {
      if (!param->write.is_prep) {
        if (config.heart_rate_handle_table[IDX_CHAR_VAL_VOL] ==
                param->write.handle &&
            param->write.len == 3) {
          /*A character */
          if (param->write.value[0] == 0x52 && param->write.value[2] == 0x4D) {
            memcpy(config.char_inst[0].char_value, &param->write.value[1],
                   sizeof(param->write.value[1]));
            /* R+value+M */
          }
        }
        if (config.heart_rate_handle_table[IDX_CHAR_CFG_VOL] ==
                param->write.handle &&
            param->write.len == 2) {
          /*A characterConfig */
          uint16_t descr_value =
              param->write.value[1] << 8 | param->write.value[0];
          if (descr_value == 0x0001) {
            uint8_t notify_data[] = {"Notice Enabled info"};
            esp_ble_gatts_send_indicate(
                gatts_if, param->write.conn_id,
                config.heart_rate_handle_table[IDX_CHAR_VAL_VOL],
                sizeof(notify_data), notify_data, false);
          } else if (descr_value == 0x0002) {
            uint8_t indicate_data[] = {0x01, 0x02};
            esp_ble_gatts_send_indicate(
                gatts_if, param->write.conn_id,
                config.heart_rate_handle_table[IDX_CHAR_VAL_VOL],
                sizeof(indicate_data), indicate_data, true);
          }
        }
        /* send response when param->write.need_srp is true*/
        if (param->write.need_rsp) {
          esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                      param->write.trans_id, ESP_GATT_OK, NULL);
        } else {
        }
      } else {
        /* handle prepare write */
        example_prepare_write_event_env(gatts_if, &config.write_env, param);
      }
    } break;
    case ESP_GATTS_EXEC_WRITE_EVT: {
      example_exec_write_event_env(&config.write_env, param);
    } break;
    case ESP_GATTS_MTU_EVT:
    case ESP_GATTS_CONF_EVT:
    case ESP_GATTS_START_EVT:
    case ESP_GATTS_CONNECT_EVT: {
      esp_ble_conn_update_params_t conn_params = {0};
      memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
      /* For the iOS system, please refer to Apple official documents about
       * the BLE connection parameters restrictions. */
      conn_params.latency = 0;
      conn_params.max_int = 0x20;  // max_int = 0x20*1.25ms = 40ms
      conn_params.min_int = 0x10;  // min_int = 0x10*1.25ms = 20ms
      conn_params.timeout = 400;   // timeout = 400*10ms = 4000ms
      // start sent the update connection parameters to the peer device.
      esp_ble_gap_update_conn_params(&conn_params);
    } break;
    case ESP_GATTS_DISCONNECT_EVT: {
      esp_ble_gap_start_advertising(&config.adv_params);
    } break;
    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
      if (param->add_attr_tab.status != ESP_GATT_OK &&
          param->add_attr_tab.num_handle != HRS_IDX_NB) {
      } else {
        memcpy(config.heart_rate_handle_table, param->add_attr_tab.handles,
               sizeof(config.heart_rate_handle_table));
        esp_ble_gatts_start_service(config.heart_rate_handle_table[IDX_SVC]);
      }
    } break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
    case ESP_GATTS_STOP_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    case ESP_GATTS_UNREG_EVT:
    case ESP_GATTS_DELETE_EVT:
    default:
      break;
  }
}

static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param) {
  /* If event is register event, store the gatts_if for each profile */
  if (event == ESP_GATTS_REG_EVT) {
    if (param->reg.status == ESP_GATT_OK) {
      heart_rate_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
    }
  }
  do {
    int idx;
    for (idx = 0; idx < PROFILE_NUM; idx++) {
      /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call
       * every profile cb function */
      if (gatts_if == ESP_GATT_IF_NONE ||
          gatts_if == heart_rate_profile_tab[idx].gatts_if) {
        if (heart_rate_profile_tab[idx].gatts_cb) {
          heart_rate_profile_tab[idx].gatts_cb(event, gatts_if, param);
        }
      }
    }
  } while (0);
}

void bsp_ble_init() {
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);
  esp_bluedroid_init();
  esp_bluedroid_enable();
  esp_ble_gatts_register_callback(gatts_event_handler);
  esp_ble_gap_register_callback(gap_event_handler);
  esp_ble_gatts_app_register(ESP_APP_ID);
  esp_ble_gatt_set_local_mtu(500);
}

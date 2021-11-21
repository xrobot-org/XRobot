/**
 * @file thd_usb.c
 * @author Qu Shen
 * @brief USB输入输出管理
 * @version 0.1
 * @date 2021-09-05
 *
 * @copyright Copyright (c) 2021
 *
 */

#include <string.h>

#include "thd.h"
#include "tusb.h"

#define THD_PERIOD_MS (2)

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  ;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  ;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  ;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  ;
}


//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void)
{
  if ( tud_cdc_connected() )
  {
    // connected and there is data available
    if ( tud_cdc_available() )
    {
      uint8_t buf[64];

      // read and echo back
      uint32_t count = tud_cdc_read(buf, 1);

      if (count == 0) {
        return;
      }
      volatile uint8_t command = buf[0];
      if ((command & 0x80) == 0x80) {
        tud_cdc_read(buf, 4);
      }
      switch (command) {
        case 0x00: // Reset
          break;
        case 0x01: // Run
          ;
          break;
        case 0x02: { // ID
          const char id[] = "TLF1";
          tud_cdc_write(id, 4);
          // tud_cdc_write_char('1');
          // tud_cdc_write_char('A');
          // tud_cdc_write_char('L');
          // tud_cdc_write_char('S');
          break;
        }
        case 0x04: { // metadata
          tud_cdc_write_char(0x1);
          tud_cdc_write_str("device name");
          tud_cdc_write_char(0);
          tud_cdc_write_char(0x2);
          tud_cdc_write_str("fpga firmware");
          tud_cdc_write_char(0);
          tud_cdc_write_char(0x3);
          tud_cdc_write_str("pic firmware");
          tud_cdc_write_char(0);
          tud_cdc_write_char(0x40);
          tud_cdc_write_char(4); // number of usable probes
          tud_cdc_write_char(0x41);
          tud_cdc_write_char(2); // protocol version
          tud_cdc_write_char(0);
          break;
        }
        case 0xc0: { // set trigger mask
          break;
        }
        case 0xc1: { // set trigger values
          break;
        }
        case 0xc2: { // set trigger configuration
          break;
        }
        case 0x80: { // set divider
          break;
        }
        case 0x81: { // set read and delay count
          break;
        }
        case 0x82: { // set flags
          break;
        }
        default:
          //asm("bkpt");
          break;
      }

      // for(uint32_t i=0; i<count; i++)
      // {
      //   tud_cdc_write_char(buf[i]);

      //   if ( buf[i] == '\r' ) tud_cdc_write_char('\n');
      // }

    }
    tud_cdc_write_flush();
  }
}

void tlf_queue_sample(uint8_t* sample, uint32_t sample_len) {
    tud_cdc_write(sample, sample_len);
    // if (sent == 0) {
    //   asm("bkpt");
    // }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void) itf;
  (void) dtr;
  (void) rts;
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
  (void) itf;
}

void Thd_USB(void* arg) {
  Runtime_t* runtime = arg;
  RM_UNUSED(runtime);

  // This should be called after scheduler/kernel is started.
  // Otherwise it could cause kernel issue since USB IRQ handler does use RTOS
  // queue API.
  tusb_init();

  // RTOS forever loop
  while (1) {
    // tinyusb device task
    tud_task();

    cdc_task();
  }
}

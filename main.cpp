/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.hpp"
#include "hal.h"
//#include "RTT/SEGGER_RTT.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "shell.h"

#include "chprintf.h"
using namespace chibios_rt;

#include "adc.h"
#include "pwm.h"
#include "foc.h"
#include "rtt.h"
#include "uavcan.h"

/*
 * Green LED blinker thread, times are in milliseconds.
 */

unsigned drv8320_read(unsigned addr)
{
    uint16_t x = (1 << 15) | (addr << 11), y;
    spiSelect(&SPID1);
    y = spiPolledExchange(&SPID1, x);          /* Atomic transfer operations.      */
    spiUnselect(&SPID1);
    return y;
}

unsigned drv8320_write(unsigned addr, unsigned data)
{
    uint16_t x = (addr << 11) | (data & 0x3ffu), y;
    spiSelect(&SPID1);
    y = spiPolledExchange(&SPID1, x);          /* Atomic transfer operations.      */
    spiUnselect(&SPID1);
    return y;
}

static void cmd_write(BaseSequentialStream *chp, int argc, char *argv[]) {
    unsigned x = 0, addr = 0;
    if (argc == 2) {
        addr = atoi(argv[0]);
        x = strtol(argv[1], NULL, 16);
    }
  drv8320_write(addr, x);
  for (addr = 0; addr < 6; ++addr) {
    chprintf(chp, "value[%d] = %02x\r\n", addr, drv8320_read(addr));
  }
}

static void cmd_pwm(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)chp;
    unsigned x = 0, addr = 0;
    if (argc == 2) {
        addr = strtol(argv[0], NULL, 10);
        x = strtol(argv[1], NULL, 10);
        pwm_set_channel(addr, x);
    } else
        for (unsigned i = 0; i < 3; ++i) {
            pwm_set_channel(i, 0);
        }
}

static void cmd_id(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)chp;
    int x = 0;
    if (argc == 1) {
        x = strtol(argv[0], NULL, 10);
        foc.id_req = x;
        chThdSleepMilliseconds(1000);
    }
    foc.id_req = 0.0f;
}
static void cmd_iq(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)chp;
    int x = 0;
    if (argc == 1) {
        x = strtol(argv[0], NULL, 10);
        foc.iq_req = x;
        chThdSleepMilliseconds(1000);
    }
    foc.iq_req = 0.0f;
}

static void cmd_adc(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;
  chprintf(chp, "IA=%.2f\r\n", (double)adc.Ia());
  chprintf(chp, "IB=%.2f\r\n", (double)adc.Ib());
  chprintf(chp, "VA=%.2f\r\n", (double)adc.Va());
  chprintf(chp, "VB=%.2f\r\n", (double)adc.Vb());
  chprintf(chp, "VC=%.2f\r\n", (double)adc.Vc());
  chprintf(chp, "VBUS=%.2f\r\n", (double)adc.Vbus());
  chprintf(chp, "TEMP=%.2f\r\n", (double)adc.T());
}

#if 0
static uint16_t crcAdd(uint16_t crc_val, const uint8_t *bytes, size_t len);
static void cmd_crc(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;
    uint8_t bytes[] = "123456789";
    chprintf(chp, "crc=%x\r\n", crcAdd(0xffffu, bytes, 9));
}
#endif
static void cmd_enable(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)chp;
    unsigned x = 0;
    if (argc) {
        x = atoi(argv[0]);
    }
    if (x)
        palSetLine(LINE_ENABLE);
    else
        palClearLine(LINE_ENABLE);
}

struct UAVCAN uavcan;

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)
static const ShellCommand commands[] = {
  {"write", cmd_write},
  {"adc", cmd_adc},
  //{"crc", cmd_crc},
  {"enable", cmd_enable},
  {"pwm", cmd_pwm},
  {"id", cmd_id},
  {"iq", cmd_iq},
  {NULL, NULL}
};

static const ShellConfig shell_cfg = {
  //(BaseSequentialStream *)&rtt,
  (BaseSequentialStream *)&SD1,
  commands
};

static const SPIConfig spicfg = {
  .circular = false,
  .end_cb = NULL,
  .ssline = LINE_SPI_NSS,
  .cr1 = SPI_CR1_BR_1 | SPI_CR1_CPHA, // 9 MHz
  .cr2 = (15 << SPI_CR2_DS_Pos)
};

/*
 * Application entry point.
 */
static void drv8320_init() {
    palSetLine(LINE_ENABLE);
    chThdSleepMilliseconds(10);
    drv8320_write(2, 0x21);
}

int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  System::init();
  /*
   * Activates the serial driver 2 using the driver default configuration.
   */
  sdStart(&SD1, NULL);
  spiStart(&SPID1, &spicfg);       /* Setup transfer parameters.       */
  getUniqueId(&uavcan);
  uavcanPoolsInit();
  crc16Init();
  drv8320_init();
  pwm_init();
  pwm_start();
/*
  pwm_set_channel(0, 100);
  pwm_set_channel(1, 200);
  pwm_set_channel(2, 400);
*/
  /*
   * Creates the blinker thread.
   */
  foc.thread = chThdCreateStatic(waThread1, sizeof(waThread1), HIGHPRIO, Thread1, NULL);

  /*
   * Shell manager initialization.
   */
  shellInit();

  /*
   * Activates the CAN driver 1.
   */
  canStart(&CAND1, &cancfg);

  /*
   * Starting the transmitter and receiver threads.
   */
  chThdCreateStatic(can_rx_wa, sizeof(can_rx_wa), NORMALPRIO + 7, can_rx, &uavcan);
  chThdCreateStatic(can_tx_wa, sizeof(can_tx_wa), NORMALPRIO + 7, can_tx, &uavcan);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                              "shell", NORMALPRIO + 1,
                                              shellThread, (void *)&shell_cfg);
    chThdWait(shelltp);               /* Waiting termination.             */
    chThdSleepMilliseconds(1000);
  }
}

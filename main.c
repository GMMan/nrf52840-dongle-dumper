/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 * @defgroup uart_example_main main.c
 * @{
 * @ingroup uart_example
 * @brief UART Example Application main file.
 *
 * This file contains the source code for a sample application using UART.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "bsp.h"
#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#include "shrink/shrink.h"

#ifdef BOARD_PCA10059

#define RX_PIN_NUMBER  UART_PIN_DISCONNECTED
#define TX_PIN_NUMBER  15
#define CTS_PIN_NUMBER UART_PIN_DISCONNECTED
#define RTS_PIN_NUMBER UART_PIN_DISCONNECTED

#endif

//#define ENABLE_LOOPBACK_TEST  /**< if defined, then this example will be a loopback test, which means that TX should be connected to RX to get data loopback. */

#define MAX_TEST_DATA_BYTES     (15U)                /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256                         /**< UART RX buffer size. */

void uart_event_handle(app_uart_evt_t * p_event)
{
    
}


/* When UART is used for communication with the host do not use flow control.*/
#define UART_HWFC APP_UART_FLOW_CONTROL_DISABLED

struct output_ctx_t {
    const uint8_t *ptr;
    size_t length;
    size_t offset;
};

void uart_put(uint8_t byte)
{
    ret_code_t ret;
    do {
        ret = app_uart_put(byte);
    } while (ret != NRF_SUCCESS);
}

void uart_put_int(uint32_t num)
{
    uart_put((uint8_t)(num));
    uart_put((uint8_t)(num >> 8));
    uart_put((uint8_t)(num >> 16));
    uart_put((uint8_t)(num >> 24));
}

int output_get(void *in)
{
    struct output_ctx_t *ctx = in;

    if (ctx->offset >= ctx->length) return -1;

    return ctx->ptr[ctx->offset++];
}

int output_put(int ch, void *out)
{
    uart_put((uint8_t)ch);
    return ch;
}

void output_region(const uint8_t *ptr, size_t length)
{
    // RLE compress data
    struct output_ctx_t ctx = {
        .ptr = ptr,
        .length = length,
        .offset = 0
    };

    // Output marker
    uart_put_int(0x0dd0aa55);
    uart_put_int((uint32_t)ptr);
    uart_put_int(length);

    shrink_t shrink_io = {
        .get = output_get,
        .put = output_put,
        .in = &ctx,
        .out = &ctx,
        .read = 0,
        .wrote = 0
    };
    shrink(&shrink_io, CODEC_RLE, 1);

    uart_put_int(0x55aad00d);
}

/**
 * @brief Function for main application entry.
 */
int main(void)
{
    uint32_t err_code;

    //bsp_board_init(BSP_INIT_LEDS);

    const app_uart_comm_params_t comm_params =
      {
          RX_PIN_NUMBER,
          TX_PIN_NUMBER,
          RTS_PIN_NUMBER,
          CTS_PIN_NUMBER,
          UART_HWFC,
          false,
#if defined (UART_PRESENT)
          NRF_UART_BAUDRATE_115200
#else
          NRF_UARTE_BAUDRATE_115200
#endif
      };

    APP_UART_INIT(&comm_params,
                    uart_event_handle,
                    APP_IRQ_PRIORITY_LOWEST,
                    err_code);

    //APP_ERROR_CHECK(err_code);
    (void)err_code;

    while (true)
    {
        output_region((uint8_t *)0, 0x100000);
        output_region((uint8_t *)NRF_FICR_BASE, 0x1000);
        output_region((uint8_t *)NRF_UICR_BASE, 0x1000);
    }
}


/** @} */

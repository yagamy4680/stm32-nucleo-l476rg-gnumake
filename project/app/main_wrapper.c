/**
 * Copyright 2014-2015 T2T Inc. All rights reserved.
 */

/*=======================================================================
 *  FILE:
 *      main_wrapper.c
 *
 *
 *  DESCRIPTION:
 *      Wrapper functions for main.c
 *
 *
 *  REVISION HISTORY:
 *      2019/11/13, yagamy, initial version.
 *
 *
 *=======================================================================*/
#define MODULE_NAME "main_wrapper.c"


/*-----------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "stm32l4xx_hal.h"
#include "main_wrapper.h"

/*-----------------------------------------------------------------------
 * Marcos, Types, and Structures
 *-----------------------------------------------------------------------*/
#define MAX_TX_BUFFER_SIZE            (1024)
#define MAX_RX_BUFFER_SIZE            (32)
#define MAX_UART_COUNT                (MAX_UART_PERIPHERAL_COUNT + 1)     /* 0, 1, 2, 3, 4. Please note, `0` is reserved as NULL. */


/*-----------------------------------------------------------------------
 * Global Variables
 *-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------
 * Global External Functions and Variables
 *-----------------------------------------------------------------------*/
extern void tt_uart_command_init(void);
extern int tt_uart_command_handler(int i, char *command);

/*-----------------------------------------------------------------------
 * Local Function Prototypes
 *-----------------------------------------------------------------------*/
static HAL_StatusTypeDef tt_uart_send(int console, char *str);
static void print_welcome_messages(void);
static int main_loop_body(Context *ctx);

/*-----------------------------------------------------------------------
 * Local Variables
 *-----------------------------------------------------------------------*/
static int m_counter = 0;
static char m_tx_buffer[MAX_TX_BUFFER_SIZE] = {0};
static char m_rx_buffer[MAX_RX_BUFFER_SIZE] = {0};
static int m_rx_recv_bytes = 0;
static int m_rx_uart_peripheral = 0;
static UART_HandleTypeDef *m_console = NULL;
static UART_HandleTypeDef *m_uarts[MAX_UART_COUNT] = {0};       // 0, 1, 2, 3, 4. Please note, `0` is reserved as NULL.


/*-----------------------------------------------------------------------
 * Global Functions
 *-----------------------------------------------------------------------*/
int main_wrapper(Context *context)
{
    Peripheral *cp = NULL;

    if (!context) return 1;
    if (!(context->system_clock_config)) return 2;
    if (!(context->gpio_init)) return 3;

    cp = &(context->console);
    if (!(cp->p)) return 4;

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    context->system_clock_config();

    /* Initialize Console-UART peripheral, and print welcome messages */
    m_console = (UART_HandleTypeDef *) cp->p;
    cp->init();
    print_welcome_messages();

    /* Initialize GPIOs */
    context->gpio_init();

    /* Initialize UARTs */
    for (int i = 0; i < MAX_UART_PERIPHERAL_COUNT; i++) {
        Peripheral *pp = &(context->uart_instances[i]);
        if (!(pp->p)) {
            DBG("uart%d: ignored", i + 1);
            continue;
        }
        m_uarts[i + 1] = (UART_HandleTypeDef *) pp->p;
        if (cp->p == pp->p) {
            /* ignore console uart because it has been initialized before */
            DBG("uart%d: initialized (console)", i + 1);
        }
        else {
            pp->init();
            DBG("uart%d: initialized", i + 1);
        }
    }

    /* Initialize I2Cs */
    for (int i = 0; i < MAX_I2C_PERIPHERAL_COUNT; i++) {
        Peripheral *pp = &(context->i2c_instances[i]);
        if (!(pp->p)) {
            DBG("i2c%d : ignored", i + 1);
            continue;
        }
        pp->init();
        DBG("i2c%d : initialized", i + 1);
    }

    return main_loop_body(context);
}

HAL_StatusTypeDef tt_uart_sendPacket0(int console, uint8_t prefix, uint8_t timefmt, const char *fmt, ...)
{
    int length = sizeof(m_tx_buffer);
    char *ptr = m_tx_buffer;
    va_list args;
    HAL_StatusTypeDef result = HAL_ERROR;

    if (prefix < 0x20) return result;
    if (prefix > 0x7F) return result;
    if (!fmt) return result;

    if (0 == strlen(fmt)) {
        // Print a blank line
        m_tx_buffer[0] = '\r';
        m_tx_buffer[1] = '\n';
        m_tx_buffer[2] = 0;
    }
    else {
        // Print a line with formatted strings
        memset(m_tx_buffer, 0, MAX_TX_BUFFER_SIZE);
        switch (timefmt)
        {
            case DEBUG_TIME_FORMAT_UPTIME_MS:
            case DEBUG_TIME_FORMAT_EPOCH_MS:
                snprintf(m_tx_buffer, length - 3, "%c%c%u%c%lu%c", prefix, TOKEN_SEPARATOR, m_counter++, TOKEN_SEPARATOR, HAL_GetTick(), TOKEN_SEPARATOR);
                break;
    
            case DEBUG_TIME_FORMAT_NONE:
            default:
                snprintf(m_tx_buffer, length - 3, "%c%c", prefix, TOKEN_SEPARATOR);
            break;
        }

        length = strlen(m_tx_buffer);
        ptr = m_tx_buffer + length;

        va_start(args, fmt);
        vsnprintf(ptr, sizeof(m_tx_buffer) - (length + 3), fmt, args);
        va_end(args);

        length = strlen(m_tx_buffer);
        m_tx_buffer[length + 0] = '\r';
        m_tx_buffer[length + 1] = '\n';
        m_tx_buffer[length + 2] = 0;
    }

    result = tt_uart_send(console, m_tx_buffer);
    return result;
}

void stm32l4xx_it_wrapper_process_data(UART_HandleTypeDef *huart, char *buffer, int size)
{
    int i = 0;

    if (size <= 0) {
        return;
    }

    if (!huart) {
        return;
    }

    for (i = 0; i < MAX_UART_COUNT; i++) {
        if (m_uarts[i] == huart) break;
    }

    if (i >= MAX_UART_COUNT) {
        // Cannot find the UART device from device list.
        return;
    }
    
    m_rx_uart_peripheral = i;
    m_rx_recv_bytes = size;
    memset(m_rx_buffer, 0, MAX_RX_BUFFER_SIZE);
    memcpy(m_rx_buffer, buffer, size);

    if (m_rx_buffer[size - 1] == '\n') {
        m_rx_buffer[size - 1] = 0;
    }

    if (m_rx_buffer[size - 2] == '\r') {
        m_rx_buffer[size - 2] = 0;
    }

    if (tt_uart_command_handler(i, m_rx_buffer)) {
        m_rx_uart_peripheral = 0;
        m_rx_recv_bytes = 0;
    }
}

/*-----------------------------------------------------------------------
 * Local Functions
 *-----------------------------------------------------------------------*/

/**
 * @brief Send a string to the specified console(s) UART instances.
 * 
 * @param console `0` to the default console uart. `1` ~ `5` to the usart1, usart2, usart3, uart4, uart5
 * @param str     The string to be send to UART instance.
 * @return HAL_StatusTypeDef 
 */
static HAL_StatusTypeDef tt_uart_send(int console, char *str)
{
    HAL_StatusTypeDef result = HAL_OK;
    uint16_t length;
    uint8_t *ptr = (uint8_t *) str;

    if (!str) return HAL_ERROR;
    length = strlen(str);

    if (0 == console) {
        return HAL_UART_Transmit(m_console, ptr, length, MAX_FLUSH_CONSOLE_TIMEOUT);
    }
    else if (0 > console) {
        for (int i = 1; i < MAX_UART_COUNT; i++) {
            UART_HandleTypeDef *huart = m_uarts[i];
            if (huart) {
                result = HAL_UART_Transmit(huart, ptr, length, MAX_FLUSH_CONSOLE_TIMEOUT);
                if (HAL_OK != result) return result;
            }
        }
        return HAL_OK;
    }
    else if (MAX_UART_COUNT > console) {
        return HAL_UART_Transmit(m_uarts[console], ptr, length, MAX_FLUSH_CONSOLE_TIMEOUT);
    }
    else {
        return HAL_ERROR;
    }
}

static void print_welcome_messages(void)
{
    char tmp[32] = {0};
    int len = strlen(PROJECT_NAME);

    for (int i = 0; i < 32; i++) {
        if (i < len) {
            tmp[i] = '-';
        }
    }
    tmp[sizeof(tmp)] = 0;

    WELCOME("");
    WELCOME("");
    WELCOME(tmp);
    WELCOME("%s", PROJECT_NAME);
    WELCOME(tmp);
    WELCOME("build-info%c%s", TOKEN_SEPARATOR, VERSION_BUILD_INFO);
    WELCOME("build-date%c%s", TOKEN_SEPARATOR, VERSION_BUILD_DATE);
    WELCOME("build-time%c%s", TOKEN_SEPARATOR, VERSION_BUILD_TIME);
    WELCOME("built-by%c%s", TOKEN_SEPARATOR, VERSION_BUILD_MACHINE);
    WELCOME("entry%c%s", TOKEN_SEPARATOR, __FILE__);
    WELCOME("");
    WELCOME(""); 
}

static int main_loop_body(Context *ctx)
{
    while (1) {
        HAL_Delay(1000);
        if (m_rx_uart_peripheral) {
            DBG("%d%c%d%c%s", m_rx_uart_peripheral, TOKEN_SEPARATOR, m_rx_recv_bytes, TOKEN_SEPARATOR, m_rx_buffer);
            m_rx_uart_peripheral = 0;
        }
        else {
            DBG("%s", "hello");
        }
    }
    return 0;
}
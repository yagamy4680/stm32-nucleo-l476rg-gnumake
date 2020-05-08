/**
 * Copyright (c) 2019-2020 T2T Inc. All rights reserved.
 */

#ifndef __MAIN_WRAPPER_H
#define __MAIN_WRAPPER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------------------------------------------------*/
/* Include Headers                                                         */
/*-------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/*-------------------------------------------------------------------------*/
/* Configurable Marcos                                                     */
/*-------------------------------------------------------------------------*/
#define MAX_FLUSH_CONSOLE_TIMEOUT     (3000)
#define TOKEN_SEPARATOR               ('\t')

#define MAX_UART_PERIPHERAL_COUNT     (5)
#define MAX_I2C_PERIPHERAL_COUNT      (3)
#define MAX_SPI_PERIPHERAL_COUNT      (3)


/*-------------------------------------------------------------------------*/
/* Constant Marcos                                                         */
/*-------------------------------------------------------------------------*/
#ifndef VERSION_BUILD_INFO
#define VERSION_BUILD_INFO      "dirty"
#endif

#ifndef VERSION_BUILD_DATE
#define VERSION_BUILD_DATE      "1970/01/01"
#endif

#ifndef VERSION_BUILD_TIME
#define VERSION_BUILD_TIME      "00:00:00"
#endif

#ifndef VERSION_BUILD_MACHINE
#define VERSION_BUILD_MACHINE   "ghost@machine"
#endif

#ifndef PROJECT_NAME
#define PROJECT_NAME            "stm32xxx"
#endif

#define DEBUG_TIME_FORMAT_NONE        (0)
#define DEBUG_TIME_FORMAT_UPTIME_MS   (1)
#define DEBUG_TIME_FORMAT_EPOCH_MS    (2)

/**
 * Use `token paste operator` (##) to force preprocessor to remove the
 * comma before. Refer to https://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Variadic-Macros.html#Variadic-Macros
 */
#define WELCOME(format, ...)          tt_uart_sendPacket0(0, '+', DEBUG_TIME_FORMAT_NONE, format, ## __VA_ARGS__)
#define DBG(format, ...)              tt_uart_sendPacket0(0, '#', DEBUG_TIME_FORMAT_UPTIME_MS, format, ## __VA_ARGS__)
#define INFO(format, ...)             tt_uart_sendPacket0(0, '*', DEBUG_TIME_FORMAT_UPTIME_MS, format, ## __VA_ARGS__)
#define WARN(format, ...)             tt_uart_sendPacket0(0, '!', DEBUG_TIME_FORMAT_UPTIME_MS, format, ## __VA_ARGS__)

/*-------------------------------------------------------------------------*/
/* Types and Structures                                                    */
/*-------------------------------------------------------------------------*/

/**
 * @brief Init function for peripheral.
 */
typedef void (* init_function) (void);

/**
 * @brief Fini function for peripheral.
 */
typedef void (* fini_function) (void);


/**
 * @brief Peripheral Structure.
 */
typedef struct PeripheralStruct {
    void *p;
    init_function init;
} 
Peripheral;


typedef struct ContextStruct {
    init_function system_clock_config;
    init_function gpio_init;

    Peripheral console;

    Peripheral uart_instances[MAX_UART_PERIPHERAL_COUNT];
    Peripheral i2c_instances[MAX_I2C_PERIPHERAL_COUNT];
    Peripheral spi_instances[MAX_SPI_PERIPHERAL_COUNT];
    Peripheral ltdc_instances[1];

    UART_HandleTypeDef *huart1;
    UART_HandleTypeDef *huart2;
    UART_HandleTypeDef *huart3;
    UART_HandleTypeDef *huart4;
    UART_HandleTypeDef *huart5;

    I2C_HandleTypeDef *i2c1;
    I2C_HandleTypeDef *i2c2;
    I2C_HandleTypeDef *i2c3;
}
Context;

/*-------------------------------------------------------------------------*/
/* Global Function Prototypes                                              */
/*-------------------------------------------------------------------------*/

/**
 * @brief Wrapper function for entry point `main()`.
 * 
 * @param ctx The context of peripheral instances.
 * @return int 
 */
int main_wrapper(Context *ctx);

/**
 * @brief 
 * 
 * @param console 
 * @param prefix 
 * @param timefmt 
 * @param fmt 
 * @param ... 
 * @return HAL_StatusTypeDef 
 */
HAL_StatusTypeDef tt_uart_sendPacket0(int console, uint8_t prefix, uint8_t timefmt, const char *fmt, ...);


#ifdef __cplusplus
}
#endif

#endif /* __MAIN_WRAPPER_H */

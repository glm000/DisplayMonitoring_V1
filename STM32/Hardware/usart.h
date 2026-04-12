#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include <stdio.h>

/**
 * @brief  USART1初始化函数
 * @param  baudrate: 波特率（如 9600, 115200）
 */
void USART1_Init(uint32_t baudrate);

#endif

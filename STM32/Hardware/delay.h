#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

// 函数声明（修复SysTick_Init未声明问题）
void SysTick_Init(void);
void Delay_us(u32 us);  // 微秒级延时
void Delay_ms(u32 ms);  // 毫秒级延时

#endif

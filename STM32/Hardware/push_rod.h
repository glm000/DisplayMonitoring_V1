#ifndef __PUSH_ROD_H
#define __PUSH_ROD_H

#include "stm32f10x.h"

// ================= 硬件引脚宏定义 =================

// 电机控制引脚 (接电机驱动板)
#define PUSH_ROD_PORT       GPIOB
#define PUSH_ROD_A1_PIN     GPIO_Pin_10
#define PUSH_ROD_B1_PIN     GPIO_Pin_11

// 微动开关检测引脚 (安装在推杆顶端)
#define SWITCH_PORT         GPIOB
#define SWITCH_GND_PIN      GPIO_Pin_12 // 充当GND供电
#define SWITCH_DETECT_PIN   GPIO_Pin_13 // 检测引脚

// ================= 电机控制底层宏 =================
#define PUSH_ROD_A1_HIGH()  GPIO_SetBits(PUSH_ROD_PORT, PUSH_ROD_A1_PIN)
#define PUSH_ROD_A1_LOW()   GPIO_ResetBits(PUSH_ROD_PORT, PUSH_ROD_A1_PIN)

#define PUSH_ROD_B1_HIGH()  GPIO_SetBits(PUSH_ROD_PORT, PUSH_ROD_B1_PIN)
#define PUSH_ROD_B1_LOW()   GPIO_ResetBits(PUSH_ROD_PORT, PUSH_ROD_B1_PIN)

// ================= 外部调用函数声明 =================
void PushRod_Init(void);
void PushRod_Extend(void);
void PushRod_Retract(void);
void PushRod_Stop(void);
uint8_t Is_Screen_Touched(void);
uint8_t Perform_Screen_Tap(void);

#endif

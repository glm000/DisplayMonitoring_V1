#ifndef __MYIIC_H
#define __MYIIC_H
#include "stm32f10x.h"

// ============================================
//   用户引脚定义 (当前适配: PB8 / PB9)
// ============================================
// 如果以后要改回 PB6/PB7，只需改这里的引脚和下面的 SDA_IN/OUT 宏
#define IIC_SCL_PORT    GPIOB
#define IIC_SCL_PIN     GPIO_Pin_8
#define IIC_SDA_PORT    GPIOB
#define IIC_SDA_PIN     GPIO_Pin_9

// ============================================
//   SDA 输入/输出方向切换 (关键修改!)
// ============================================
// 针对 PB9 (属于高位寄存器 CRH, 第 4-7 位)
// 0xFFFFFF0F 表示清除第 4-7 位
// 8<<4 表示输入模式 (1000)
// 3<<4 表示输出模式 (0011)
// 切换为输入模式 (上拉输入，防止悬空乱跳)
#define SDA_IN()  {GPIO_InitTypeDef GPIO_InitStructure; \
                   GPIO_InitStructure.GPIO_Pin = IIC_SDA_PIN; \
                   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; \
                   GPIO_Init(IIC_SDA_PORT, &GPIO_InitStructure);}
// 切换为输出模式 (开漏输出，兼容 5V)
#define SDA_OUT() {GPIO_InitTypeDef GPIO_InitStructure; \
                   GPIO_InitStructure.GPIO_Pin = IIC_SDA_PIN; \
                   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; \
                   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; \
                   GPIO_Init(IIC_SDA_PORT, &GPIO_InitStructure);}

// ============================================
//   底层操作宏
// ============================================
#define IIC_SCL_HIGH()  GPIO_SetBits(IIC_SCL_PORT, IIC_SCL_PIN)
#define IIC_SCL_LOW()   GPIO_ResetBits(IIC_SCL_PORT, IIC_SCL_PIN)
#define IIC_SDA_HIGH()  GPIO_SetBits(IIC_SDA_PORT, IIC_SDA_PIN)
#define IIC_SDA_LOW()   GPIO_ResetBits(IIC_SDA_PORT, IIC_SDA_PIN)
#define READ_SDA()      GPIO_ReadInputDataBit(IIC_SDA_PORT, IIC_SDA_PIN)

// ============================================
//   函数声明
// ============================================
void IIC_Init(void);                // 初始化
void IIC_Start(void);               // 发送起始信号
void IIC_Stop(void);                // 发送停止信号
void IIC_Send_Byte(u8 txd);         // 发送一个字节
u8 IIC_Read_Byte(unsigned char ack);// 读取一个字节
u8 IIC_Wait_Ack(void);              // 等待ACK
void IIC_Ack(void);                 // 发送ACK
void IIC_NAck(void);                // 发送NACK
void IIC_Write_One_Byte(u8 daddr,u8 addr,u8 data);
u8 IIC_Read_One_Byte(u8 daddr,u8 addr);

#endif

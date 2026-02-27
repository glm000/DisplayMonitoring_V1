#ifndef __OPT3001_H
#define __OPT3001_H

#include "stm32f10x.h"

/********************* IIC引脚定义（可根据硬件修改） *********************/
#define OPT3001_IIC_SCL_PIN    GPIO_Pin_7
#define OPT3001_IIC_SDA_PIN    GPIO_Pin_6
#define OPT3001_IIC_PORT       GPIOB
#define OPT3001_IIC_RCC        RCC_APB2Periph_GPIOB

/********************* OPT3001寄存器地址 *********************/
#define OPT3001_ADDR           0x44    // ADDR接GND时的从机地址（接VDD=0x45，接SDA=0x46）
#define OPT3001_RESULT_REG     0x00    // 光照数据寄存器
#define OPT3001_CONFIG_REG     0x01    // 配置寄存器
#define OPT3001_LOW_LIMIT_REG  0x02    // 下限阈值寄存器
#define OPT3001_HIGH_LIMIT_REG 0x03    // 上限阈值寄存器

/********************* IIC底层操作宏 *********************/
#define IIC_SCL_HIGH()  GPIO_SetBits(OPT3001_IIC_PORT, OPT3001_IIC_SCL_PIN)
#define IIC_SCL_LOW()   GPIO_ResetBits(OPT3001_IIC_PORT, OPT3001_IIC_SCL_PIN)
#define IIC_SDA_HIGH()  GPIO_SetBits(OPT3001_IIC_PORT, OPT3001_IIC_SDA_PIN)
#define IIC_SDA_LOW()   GPIO_ResetBits(OPT3001_IIC_PORT, OPT3001_IIC_SDA_PIN)
#define IIC_SDA_READ()  GPIO_ReadInputDataBit(OPT3001_IIC_PORT, OPT3001_IIC_SDA_PIN)

/********************* 函数声明（修复参数不匹配问题） *********************/
// IIC底层初始化
void OPT3001_IIC_Init(void);
// IIC起始信号
void OPT3001_IIC_Start(void);
// IIC停止信号
void OPT3001_IIC_Stop(void);
// IIC发送应答
void OPT3001_IIC_SendAck(u8 ack);
// IIC等待应答
u8 OPT3001_IIC_WaitAck(void);
// IIC发送一个字节
void OPT3001_IIC_SendByte(u8 byte);
// IIC接收一个字节（带ack参数，和实现对齐）
u8 OPT3001_IIC_ReceiveByte(u8 ack);

// OPT3001寄存器写操作
u8 OPT3001_WriteReg(u8 reg_addr, u16 data);
// OPT3001寄存器读操作
u16 OPT3001_ReadReg(u8 reg_addr);
// OPT3001传感器初始化
u8 OPT3001_Init(void);
// 读取光照强度（单位：lux）
float OPT3001_ReadLux(void);


/********************* 异常处理相关定义 *********************/
#define OPT3001_MAX_RETRY    3       // 通信异常重试次数
#define OPT3001_MIN_VAL      0.01f   // 传感器最小有效量程
#define OPT3001_MAX_VAL      83886.08f // 传感器最大有效量程
#define OPT3001_JUMP_THRESH  500.0f  // 跳变阈值（可根据场景调整）
#define FILTER_WINDOW_SIZE   3       // 滑动窗口大小（3~5为宜）

// 传感器状态枚举
typedef enum {
    OPT3001_STATUS_NORMAL,   // 正常
    OPT3001_STATUS_COMM_ERR, // 通信错误
    OPT3001_STATUS_RANGE_ERR,// 量程异常
    OPT3001_STATUS_JUMP_ERR  // 跳变异常
} OPT3001_StatusTypeDef;

// 新增带异常处理的读取函数声明
float OPT3001_ReadLux_WithFilter(void);
// 获取传感器状态（用于故障排查）
OPT3001_StatusTypeDef OPT3001_GetStatus(void);

#endif

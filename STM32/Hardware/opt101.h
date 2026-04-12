#ifndef __OPT101_H
#define __OPT101_H

#include "stm32f10x.h"

// 硬件引脚定义 (使用 PA0 作为 ADC1 的通道 0)
#define OPT101_ADC_PIN       GPIO_Pin_0
#define OPT101_ADC_PORT      GPIOA
#define OPT101_ADC_RCC       RCC_APB2Periph_GPIOA
#define OPT101_ADC_CH        ADC_Channel_0

// 采样配置
#define OPT101_SAMPLE_RATE_HZ 10000 // 采样率 10kHz (分辨率 100us)
#define OPT101_BUFFER_SIZE    2000  // 缓冲区大小，记录 200ms 的波形

// 响应时间分析结果结构体
typedef struct {
    uint8_t  valid;       // 捕获是否有效 (1:检测到有效跳变, 0:未检测到跳变或噪声过大)
    uint8_t  is_rising;   // 1: 上升沿 (暗到亮), 0: 下降沿 (亮到暗)
    uint16_t v_min;       // 稳态最小值 (ADC原生值 0-4095)
    uint16_t v_max;       // 稳态最大值 (ADC原生值 0-4095)
    float    time_ms;     // 10% ~ 90% 灰阶响应时间 (毫秒)
} OPT101_ResultTypeDef;

// 外部引用的缓冲区，方便主函数打印或通过串口传给上位机
extern uint16_t opt101_adc_buffer[OPT101_BUFFER_SIZE];

// 函数声明
uint8_t OPT101_Init(void);
void OPT101_StartCapture(void);
uint8_t OPT101_IsCaptureDone(void);
OPT101_ResultTypeDef OPT101_AnalyzeBuffer(void);

#endif

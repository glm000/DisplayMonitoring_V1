#include "delay.h"

static u8  fac_us = 0;  // 微秒延时倍乘数
static u16 fac_ms = 0;  // 毫秒延时倍乘数

// SysTick初始化（加static消除原型警告，仅本文件使用）
void SysTick_Init(void)
{
    // 选择HCLK/8作为SysTick时钟源（72MHz系统时钟下，SysTick时钟为9MHz）
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
    // 计算1us需要的时钟周期数：9MHz = 9个周期/1us
    fac_us = SystemCoreClock / 8000000;
    // 计算1ms需要的时钟周期数
    fac_ms = (u16)fac_us * 1000;
}

// 微秒级延时（精度：1us）
void Delay_us(u32 us)
{
    u32 temp;
    // 加载延时计数值
    SysTick->LOAD = (u32)us * fac_us;
    // 清空当前计数值
    SysTick->VAL  = 0x00;
    // 启动SysTick定时器
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    // 等待计数完成（COUNTFLAG位置1）
    do
    {
        temp = SysTick->CTRL;
    } while((temp & 0x01) && !(temp & (1 << 16)));
    // 关闭定时器
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    // 清空计数值
    SysTick->VAL  = 0x00;
}

// 毫秒级延时（精度：1ms）
void Delay_ms(u32 ms)
{
    u32 temp;
    // 加载延时计数值
    SysTick->LOAD = (u32)ms * fac_ms;
    // 清空当前计数值
    SysTick->VAL  = 0x00;
    // 启动SysTick定时器
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    // 等待计数完成
    do
    {
        temp = SysTick->CTRL;
    } while((temp & 0x01) && !(temp & (1 << 16)));
    // 关闭定时器
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    // 清空计数值
    SysTick->VAL  = 0x00;
}

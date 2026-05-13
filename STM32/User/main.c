#include "stm32f10x.h"
#include "push_rod.h"  //
#include "delay.h"     //
#include "usart.h"     //
#include <stdio.h>

/**
 * @brief  主函数：自动化屏幕点击器逻辑
 */
int main(void)
{
    // 1. 硬件基础初始化
    Delay_Init();                // 初始化 SysTick 延时系统
    USART1_Init(115200);         // 初始化串口1，波特率 115200
    PushRod_Init();              // 初始化电机引脚与微动开关

    printf("\r\n====================================\r\n");
    printf("系统启动：自动屏幕点击测试治具 V1.1\r\n");
    printf("状态：正在进行上电安全检查...\r\n");

    // 2. 上电安全复位
    // 强制回缩 1 秒，确保推杆处于安全起始位置，不在最顶端死区
    printf("动作：推杆强制回缩中...\r\n");
    PushRod_Retract();           //
    Delay_ms(1000);              //
    PushRod_Stop();              //
    
    printf("状态：自检完成，准备进入主循环。\r\n");
    printf("设定参数：每 5 秒钟（5000ms）点击一次屏幕。\r\n");
    printf("====================================\r\n\r\n");

    while(1)
    {
        printf("状态：开始执行屏幕点击动作...\r\n");
        
        // 根据底层的执行结果，决定打印什么信息
        if (Perform_Screen_Tap() == 1) 
        {
            printf("状态：[成功] 点击完成，推杆已安全回缩。\r\n");
        } 
        else 
        {
            printf("状态：[失败] 触发了安全保护机制，推杆已强制复位。\r\n");
        }
        
        printf("状态：进入 5 秒钟睡眠等待期...\r\n");
        Delay_ms(5000); 
        
        printf("\r\n--- 等待结束，准备下一轮测试 ---\r\n");
    }
}

#include "stm32f10x.h"
#include "opt3001.h"
#include "delay.h"   
#include "stdio.h"

// 声明USART1初始化函数
void USART1_Init(u32 baudrate);

// 重定向printf到串口（屏蔽未使用参数，消除警告）
int fputc(int ch, FILE *f)
{
    (void)f; // 屏蔽未使用的f参数
    USART_SendData(USART1, (u8)ch);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}

// USART1初始化（波特率可配置）
void USART1_Init(u32 baudrate)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    
    // 使能USART1和GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置PA9（TX）为复用推挽输出
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 配置PA10（RX）为浮空输入
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 配置USART1参数
    USART_InitStruct.USART_BaudRate = baudrate;                  // 波特率
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;     // 8位数据位
    USART_InitStruct.USART_StopBits = USART_StopBits_1;          // 1位停止位
    USART_InitStruct.USART_Parity = USART_Parity_No;             // 无校验
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // 收发模式
    USART_Init(USART1, &USART_InitStruct);
    
    // 使能USART1
    USART_Cmd(USART1, ENABLE);
}

float lux;
int main(void)
{
    OPT3001_StatusTypeDef sensor_status;

    SysTick_Init();
    USART1_Init(115200);

    if(OPT3001_Init() == 0)
        printf("OPT3001初始化成功！\r\n");
    else
        printf("OPT3001初始化失败！\r\n");

    while(1)
    {
        // 调用带异常处理的读取函数
        lux = OPT3001_ReadLux_WithFilter();
        // 获取传感器状态，便于调试
        sensor_status = OPT3001_GetStatus();

			
			
        // 打印结果+状态（可选，用于故障排查）
        switch(sensor_status)
        {
            case OPT3001_STATUS_NORMAL:
                printf("当前光照强度：%.2f lux（正常）\r\n", (double)lux);
                break;
            case OPT3001_STATUS_COMM_ERR:
                printf("当前光照强度：%.2f lux（通信异常）\r\n", (double)lux);
                break;
            case OPT3001_STATUS_RANGE_ERR:
                printf("当前光照强度：%.2f lux（量程异常）\r\n", (double)lux);
                break;
            case OPT3001_STATUS_JUMP_ERR:
                printf("当前光照强度：%.2f lux（跳变异常）\r\n", (double)lux);
                break;
        }
        
        Delay_ms(100);
    }
}

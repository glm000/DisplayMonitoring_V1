#include "push_rod.h"
#include "delay.h" 
#include <stdio.h>

/**
 * @brief  初始化推杆电机与微动开关引脚
 */
void PushRod_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 1. 使能 GPIOB 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // 2. 初始化电机控制引脚 PB10, PB11 (通用推挽输出)
    GPIO_InitStructure.GPIO_Pin = PUSH_ROD_A1_PIN | PUSH_ROD_B1_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PUSH_ROD_PORT, &GPIO_InitStructure);

    // 3. 初始化开关"接地"引脚 PB12 (推挽输出)
    GPIO_InitStructure.GPIO_Pin = SWITCH_GND_PIN;
    // 复用上面的推挽输出配置
    GPIO_Init(SWITCH_PORT, &GPIO_InitStructure);
    // 强制输出低电平，模拟 GND，为微动开关提供低电势
    GPIO_ResetBits(SWITCH_PORT, SWITCH_GND_PIN);

    // 4. 初始化开关检测引脚 PB13 (上拉输入)
    GPIO_InitStructure.GPIO_Pin = SWITCH_DETECT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 内部上拉
    GPIO_Init(SWITCH_PORT, &GPIO_InitStructure);

    // 5. 初始状态电机停止，确保上电安全
    PushRod_Stop();
}

/**
 * @brief  推杆基础动作函数
 */
void PushRod_Extend(void)  { PUSH_ROD_A1_HIGH(); PUSH_ROD_B1_LOW(); }
void PushRod_Retract(void) { PUSH_ROD_A1_LOW(); PUSH_ROD_B1_HIGH(); }
void PushRod_Stop(void)    { PUSH_ROD_A1_LOW(); PUSH_ROD_B1_LOW(); }

/**
 * @brief  检测微动开关是否闭合 (已适配 常开型 NO 开关)
 * @retval 1: 已接触屏幕 (按键导通，PB13被拉低为0)
 * @retval 0: 未接触 (按键断开，PB13靠内部上拉为1)
 */
uint8_t Is_Screen_Touched(void)
{
    // 如果读取到低电平，说明常开开关闭合了
    if (GPIO_ReadInputDataBit(SWITCH_PORT, SWITCH_DETECT_PIN) == Bit_RESET)
    {
        Delay_ms(10); // 延时 10ms 软件消抖，过滤机械触点弹跳
        if (GPIO_ReadInputDataBit(SWITCH_PORT, SWITCH_DETECT_PIN) == Bit_RESET)
        {
            return 1; // 确认碰到屏幕
        }
    }
    return 0; // 还没碰到
}

/**
 * @brief  执行一次极速屏幕点按动作
 * @retval 1: 点击成功  0: 超时失败
 */
uint8_t Perform_Screen_Tap(void)
{
    uint32_t timeout_cnt = 0;
    // 超时阈值，如果推杆一直走不到底，可以适当调大这个值
    const uint32_t TIMEOUT_MAX = 5000000; 

    printf("  ---> [推杆动作] 1. 电机正转，推杆开始前伸...\r\n");
    PushRod_Extend();

    // 2. 实时检测微动开关
    while (GPIO_ReadInputDataBit(SWITCH_PORT, SWITCH_DETECT_PIN) == Bit_SET)
    {
        timeout_cnt++;
        if(timeout_cnt > TIMEOUT_MAX)
        {
            PushRod_Stop();
            printf("  ---> [严重警告] 传感器检测超时！推杆可能未对准或距离太远！\r\n");
            
            // 【关键修复】：超时后必须强制回缩，防止下次启动时累加行程撞碎屏幕
            printf("  ---> [安全动作] 紧急回缩推杆复位...\r\n");
            Delay_ms(50);     // 刹车死区
            PushRod_Retract();
            Delay_ms(5000);   // 强制退回原点
            PushRod_Stop();
            
            return 0; // 返回失败标志
        }
    }

    // 3. 正常触发
    printf("  ---> [传感器] 2. 触发低电平！已触碰屏幕，延时10ms消抖...\r\n");
    Delay_ms(20); 
    
    PushRod_Stop(); 
    printf("  ---> [推杆动作] 3. 紧急刹车，等待50ms...\r\n");
    Delay_ms(50);   
    
    printf("  ---> [推杆动作] 4. 电机反转，推杆开始回缩...\r\n");
    PushRod_Retract();
    Delay_ms(2000); 
    
    PushRod_Stop();
    printf("  ---> [推杆动作] 5. 回缩完成，电机彻底停止。\r\n");
    
    return 1; // 返回成功标志
}

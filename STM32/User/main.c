#include "stm32f10x.h"
#include "usart.h"
#include "delay.h"
#include "opt3001.h"
#include "opt101.h"
#include <stdio.h>

// 定义系统运行模式
typedef enum {
    MODE_IDLE = 0,             // 待机模式
    MODE_OPT3001_CONTINUOUS    // 连续测量亮度模式
} SystemMode_t;

int main(void)
{
    uint8_t cmd;
    float lux;
    OPT3001_StatusTypeDef sensor_status;
    OPT101_ResultTypeDef res;
    SystemMode_t current_mode = MODE_IDLE;

    // 1. 系统核心初始化
    SysTick_Init();
    USART1_Init(115200); // 推荐使用 115200 波特率，便于高速传输

    printf("\r\n==============================\r\n");
    printf("--- 屏幕光电特性测试系统 ---\r\n");
    
    // 2. 传感器初始化
    if(OPT3001_Init() == 0)
        printf("[OK] OPT3001 (亮度传感器) 初始化成功\r\n");
    else
        printf("[FAIL] OPT3001 初始化失败，请检查接线！\r\n");

// 新增对 OPT101 的判断
    if(OPT101_Init() == 0)
        printf("[OK] OPT101 (灰阶响应时间传感器) 硬件校准成功\r\n");
    else
        printf("[FAIL] OPT101 硬件初始化失败 (ADC/DMA异常)！\r\n");

    // 3. 打印指令菜单
    printf("==============================\r\n");
    printf("等待指令输入...\r\n");
    printf(" [B] : 单次测量屏幕亮度 (OPT3001)\r\n");
    printf(" [C] : 连续监控屏幕亮度 (OPT3001)\r\n");
    printf(" [R] : 捕获灰阶响应时间 (OPT101)\r\n");
    printf(" [S] : 停止连续监控并待机\r\n");
    printf("==============================\r\n");

    while(1)
    {
        // ================= 步骤 1：处理串口指令 =================
        if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
        {
            cmd = USART_ReceiveData(USART1); // 读取指令并自动清除标志位
            
            switch(cmd)
            {
                // ---- 指令 B：单次读取亮度 ----
                case 'b':
                case 'B':
                    current_mode = MODE_IDLE; // 退出可能存在的连续模式
                    lux = OPT3001_ReadLux_WithFilter();
                    sensor_status = OPT3001_GetStatus();
                    if(sensor_status == OPT3001_STATUS_NORMAL)
                        printf(">> [OPT3001] 单次亮度: %.2f lux\r\n", (double)lux);
                    else
                        printf(">> [OPT3001] 亮度读取异常 (状态码: %d)\r\n", sensor_status);
                    break;
                    
                // ---- 指令 C：连续读取亮度 ----
                case 'c':
                case 'C':
                    current_mode = MODE_OPT3001_CONTINUOUS;
                    printf(">> [系统] 进入连续亮度监控模式 (发送 'S' 停止)...\r\n");
                    break;
                    
                // ---- 指令 R：捕获灰阶响应时间 ----
                case 'r':
                case 'R':
                    current_mode = MODE_IDLE; // 停止其他任务，专注捕获
                    printf(">> [OPT101] 正在抓捕瞬态波形 (耗时 200ms)...\r\n");
                    
                    OPT101_StartCapture(); // 启动定时器和 DMA
                    
                    // 阻塞等待 200ms 录制完成
                    while(!OPT101_IsCaptureDone()); 
                    
                    res = OPT101_AnalyzeBuffer(); // 算法分析
                    
                    if(res.valid)
                    {
                        printf("---- OPT101 灰阶响应时间分析结果 ----\r\n");
                        printf("   跳变方向: %s\r\n", res.is_rising ? "上升沿 (黑到白)" : "下降沿 (白到黑)");
                        printf("   稳态基准: 谷值=%d, 峰值=%d (基于12位ADC)\r\n", res.v_min, res.v_max);
                        printf("   响应时间: %.3f ms (10%% ~ 90%%)\r\n", (double)res.time_ms);
                        printf("---------------------------------------\r\n");
                    }
                    else
                    {
                        printf(">> [OPT101] 分析失败：未检测到明显跳变，或环境噪声过大。\r\n");
                    }
                    break;
                    
                // ---- 指令 S：停止与待机 ----
                case 's':
                case 'S':
                    current_mode = MODE_IDLE;
                    printf(">> [系统] 已进入待机状态。\r\n");
                    break;
                    
                // 忽略回车换行符
                case '\r':
                case '\n':
                    break;
                    
                default:
                    printf(">> [系统] 未知指令: %c\r\n", cmd);
                    break;
            }
        }
        
        // ================= 步骤 2：执行连续后台任务 =================
        if(current_mode == MODE_OPT3001_CONTINUOUS)
        {
            lux = OPT3001_ReadLux_WithFilter();
            sensor_status = OPT3001_GetStatus();
            
            if(sensor_status == OPT3001_STATUS_NORMAL)
                printf("%.2f\r\n", (double)lux); // 只打印数字，方便上位机画图
            
            // 采样间隔 100ms（与 OPT3001 的转换周期对齐）
            Delay_ms(100); 
        }
    }
}

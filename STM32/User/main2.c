#include "stm32f10x.h"
#include "usart.h"
#include "delay.h"
#include "opt3001.h"
#include "opt101.h"
#include <stdio.h>

// ==========================================
// 定义系统运行模式状态机
// ==========================================
typedef enum {
    MODE_IDLE = 0,               // 待机/停止模式：不执行任何后台任务
    MODE_OPT3001_CONTINUOUS,     // 连续亮度监控模式：定时回传 OPT3001 数据
    MODE_OPT101_VERIFY,          // 连续电压验证模式：辅助校准 OPT101 模拟电路
    MODE_OPT3001_AUTO_CONTRAST   // 【新增】自动极值对比度模式：5秒窗口捕捉黑白画面极值
} SystemMode_t;

int main(void)
{
    // --- 原有指令与状态变量 ---
    uint8_t cmd;
    float lux;
    OPT3001_StatusTypeDef sensor_status;
    OPT101_ResultTypeDef res;
    SystemMode_t current_mode = MODE_IDLE;

    // --- 【新增】自动对比度测试相关变量 ---
    uint16_t auto_sample_cnt = 0;     // 观测窗口内的采样计数器
    float auto_max_lux = 0.0f;        // 记录窗口期内的最大照度 (对应全白画面)
    float auto_min_lux = 999999.0f;   // 记录窗口期内的最小照度 (对应全黑漏光，设为极大值以便向下更新)

    // ==========================================
    // 1. 系统核心与底层硬件初始化
    // ==========================================
    SysTick_Init();
    
    // 强烈建议保持 115200 波特率。在执行 OPT101 高速波形抓取(指令 R)时，
    // 需要瞬间向上位机倾泻大量数据，低波特率会导致数据阻塞乱码。
    USART1_Init(115200); 

    printf("\r\n==============================\r\n");
    printf("--- 屏幕光电特性测试系统 ---\r\n");
    
    // ==========================================
    // 2. 传感器总线与外设初始化
    // ==========================================
    if(OPT3001_Init() == 0)
        printf("[OK] OPT3001 (数字亮度传感器) 初始化成功\r\n");
    else
        printf("[FAIL] OPT3001 初始化失败，请检查 I2C 接线！\r\n");

    if(OPT101_Init() == 0)
        printf("[OK] OPT101 (模拟响应时间传感器) 硬件校准成功\r\n");
    else
        printf("[FAIL] OPT101 硬件初始化失败 (ADC/DMA异常)！\r\n");

    // ==========================================
    // 3. 打印交互菜单
    // ==========================================
    printf("==============================\r\n");
    printf("等待上位机或终端指令输入...\r\n");
    printf(" [B] : 单次测量屏幕亮度 (OPT3001)\r\n");
    printf(" [C] : 连续监控屏幕亮度 (OPT3001)\r\n");
    printf(" [A] : 自动测算对比度 (OPT3001 5秒极值法)\r\n"); // 【新增】菜单项
    printf(" [R] : 捕获灰阶响应时间波形 (OPT101)\r\n");
    printf(" [V] : 验证 OPT101 实时输出电压 \r\n"); 
    printf(" [S] : 停止所有连续监控任务并待机\r\n");
    printf("==============================\r\n");

    // ==========================================
    // 4. 主循环：指令解析与后台状态机
    // ==========================================
    while(1)
    {
        // --------------------------------------------------
        // 步骤 A：轮询串口，处理上位机指令
        // --------------------------------------------------
        if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
        {
            cmd = USART_ReceiveData(USART1); // 读取指令并自动清除 RXNE 标志位
            
            switch(cmd)
            {
                // ---- 指令 B：单次读取静态亮度 ----
                case 'b':
                case 'B':
                    current_mode = MODE_IDLE; // 强制退出其他连续模式
                    lux = OPT3001_ReadLux_WithFilter(); // 内部已包含滤波和异常处理
                    sensor_status = OPT3001_GetStatus();
                    if(sensor_status == OPT3001_STATUS_NORMAL)
                        printf(">> [OPT3001] 单次测光亮度: %.2f lux\r\n", (double)lux);
                    else
                        printf(">> [OPT3001] 亮度读取异常 (错误状态码: %d)\r\n", sensor_status);
                    break;
                    
                // ---- 指令 C：连续输出亮度流 (主要用于画图监控) ----
                case 'c':
                case 'C':
                    current_mode = MODE_OPT3001_CONTINUOUS;
                    printf(">> [系统] 进入连续亮度监控模式 (发送 'S' 停止)...\r\n");
                    break;
                
                // ---- 【新增】指令 A：启动自动极值对比度测试 ----
                case 'a':
                case 'A':
                    current_mode = MODE_OPT3001_AUTO_CONTRAST;
                    // 重置统计算法相关的状态变量，准备迎接新的数据流
                    auto_sample_cnt = 0;       
                    auto_max_lux = 0.0f;
                    auto_min_lux = 999999.0f;
                    printf(">> [系统] 启动 5 秒极值自动测算对比度模式。\r\n");
                    printf(">> 提示：请让被测屏幕持续在【全白】和【全黑】画面之间交替闪烁。\r\n");
                    printf(">> (建议每种颜色停留 2-3 秒，系统会自动捕获极值，发送 'S' 可停止)\r\n");
                    break;

                // ---- 指令 R：捕获动态灰阶响应时间 (GTG) ----
                case 'r':
                case 'R':
                    current_mode = MODE_IDLE; // 停止其他任务，专注 CPU 资源进行数据传输
                    // OPT101_BUFFER_SIZE 通常为 2000，10kHz下耗时 200ms
                    printf(">> [OPT101] 正在抓捕瞬态波形 (耗时约 %dms)...\r\n", (OPT101_BUFFER_SIZE * 1000 / OPT101_SAMPLE_RATE_HZ));
                    
                    OPT101_StartCapture(); // 启动 TIM3 和 DMA 硬件链路进行无损抓包
                    
                    while(!OPT101_IsCaptureDone()); // 阻塞等待底层 DMA 搬运完成
                    
                    res = OPT101_AnalyzeBuffer(); // 调用工程级算法分析 10%~90% 跳变沿
                    
                    if(res.valid)
                    {
                        printf("---- OPT101 灰阶响应时间分析结果 ----\r\n");
                        printf("   跳变方向: %s\r\n", res.is_rising ? "上升沿 (黑切白)" : "下降沿 (白切黑)");
                        printf("   稳态基准: 谷值=%d, 峰值=%d (12位ADC)\r\n", res.v_min, res.v_max);
                        printf("   响应时间: %.3f ms (10%% ~ 90%%)\r\n", (double)res.time_ms);
                        printf("---------------------------------------\r\n");
                        
                        // 将原始波形数据通过高速串口倾泻给电脑，方便上位机复现曲线
                        printf("--- RAW_DATA_START ---\r\n");
                        for(int i = 0; i < OPT101_BUFFER_SIZE; i++) {
                            printf("%d\r\n", opt101_adc_buffer[i]); 
                        }
                        printf("--- RAW_DATA_END ---\r\n");
                    }
                    else
                    {
                        printf(">> [OPT101] 分析失败：未检测到有效跳变沿，或环境噪声过大引发算法拒收。\r\n");
                    }
                    break;

                // ---- 指令 V：连续输出 OPT101 模拟电压 (用于接线调试) ----
                case 'v':
                case 'V':
                    current_mode = MODE_OPT101_VERIFY;
                    printf(">> [系统] 进入 OPT101 电压监测模式 (发送 'S' 停止)...\r\n");
                    break;

                // ---- 指令 S：系统紧急停止与待机 ----
                case 's':
                case 'S':
                    current_mode = MODE_IDLE;
                    printf(">> [系统] 所有的连续任务已中止，系统进入待机。\r\n");
                    break;
                    
                // 忽略终端敲击产生的回车换行符
                case '\r':
                case '\n':
                    break;
                    
                default:
                    printf(">> [系统] 无法识别的指令: %c\r\n", cmd);
                    break;
            }
        }
        
        // --------------------------------------------------
        // 步骤 B：执行对应模式的连续后台任务
        // --------------------------------------------------
        
        // 任务 1: 纯粹的连续数据输出
        if(current_mode == MODE_OPT3001_CONTINUOUS)
        {
            lux = OPT3001_ReadLux_WithFilter();
            sensor_status = OPT3001_GetStatus();
            
            if(sensor_status == OPT3001_STATUS_NORMAL)
                printf("%.2f\r\n", (double)lux); // 仅打印数字，方便 Serial Plotter 等上位机软件直接绘图
            
            Delay_ms(100); // 必须匹配 OPT3001 内部 100ms 的积分转换周期
        }
        
        // 任务 2: 【新增】自动对比度抓取算法
        else if (current_mode == MODE_OPT3001_AUTO_CONTRAST)
        {
            lux = OPT3001_ReadLux_WithFilter();
            sensor_status = OPT3001_GetStatus();
            
            if(sensor_status == OPT3001_STATUS_NORMAL)
            {
                // [核心逻辑] 在本周期的所有采样点中，不断刷新极值记录
                if(lux > auto_max_lux) auto_max_lux = lux;
                if(lux < auto_min_lux) auto_min_lux = lux;
                
                auto_sample_cnt++;
                
                // 当收集满 50 个点 (50 * 100ms = 5秒) 时，进行一次结算
                if(auto_sample_cnt >= 50)
                {
                    printf("\r\n--- 5秒观测周期完成 (系统已自动剥离过渡态) ---\r\n");
                    printf("   白场极大亮度: %.2f lux\r\n", (double)auto_max_lux);
                    printf("   黑场极小亮度: %.2f lux\r\n", (double)auto_min_lux);
                    
                    // 进行除法运算计算对比度 (CR = L_white / L_black)
                    // 加入工业级异常保护，防止传感器盲区导致的除 0 崩溃
                    if(auto_min_lux <= 0.01f) {
                        // 如果黑场数值小于等于 OPT3001 的最低分辨率 0.01 lux，说明漏光极低
                        printf(">> 测算静态对比度: > %.0f : 1 (黑场极度暗黑，已触及数字传感下限)\r\n", (double)(auto_max_lux / 0.01f));
                    } 
                    else if (auto_max_lux < 5.0f) {
                        // 如果白场亮度异常低，说明探头没有贴紧屏幕，或者是环境光在干扰
                        printf(">> [错误] 白场亮度异常极低！请确保探头正面紧贴发光屏幕，并且未被物理遮挡。\r\n");
                    } 
                    else {
                        // 正常工况计算
                        printf(">> 测算静态对比度: %.0f : 1\r\n", (double)(auto_max_lux / auto_min_lux));
                    }
                    printf("-------------------------------------------------\r\n");
                    
                    // 结算完成后，清空累加器，无缝重置，继续侦听下一个 5 秒周期
                    auto_sample_cnt = 0;
                    auto_max_lux = 0.0f;
                    auto_min_lux = 999999.0f; 
                }
            }
            // 每次采样的节拍延时：100ms
            Delay_ms(100); 
        }
        
        // 任务 3: OPT101 模拟电压验证
        else if (current_mode == MODE_OPT101_VERIFY)
        {
            OPT101_StartCapture(); 
            
            while(!OPT101_IsCaptureDone()); 
            
            // 为了消除模拟电路的白噪声，将 2000 个高频采样点累加求平均
            uint32_t sum = 0;
            for(int i = 0; i < OPT101_BUFFER_SIZE; i++) {
                sum += opt101_adc_buffer[i];
            }
            uint16_t avg_adc = sum / OPT101_BUFFER_SIZE;
            
            // 核心公式：将 12位 ADC 数字量 (0-4095) 还原为真实电压 (0-3.3V)
            float calc_vol = ((float)avg_adc / 4095.0f) * 3.3f;
            
            // 格式化输出，方便工程师使用万用表测量探头引脚做交叉验证
            printf("ADC原始均值: %4d | 软件换算电压: %.3f V\r\n", avg_adc, (double)calc_vol);
            
            // 验证模式不需要太快刷屏，人为延时以便肉眼观察
            Delay_ms(300);
        }
    }
}

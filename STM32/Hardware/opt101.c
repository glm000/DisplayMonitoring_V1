#include "opt101.h"
#include "delay.h"

// ADC DMA 缓冲区
uint16_t opt101_adc_buffer[OPT101_BUFFER_SIZE] = {0};

/********************* 初始化 ADC、定时器和 DMA *********************/
uint8_t OPT101_Init(void)
{
	  uint32_t timeout = 0x000FFFFF; // 设置一个足够长的超时计数
	
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    
    // 1. 使能时钟
    RCC_APB2PeriphClockCmd(OPT101_ADC_RCC | RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // 2. 配置 PA0 为模拟输入
    GPIO_InitStructure.GPIO_Pin = OPT101_ADC_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(OPT101_ADC_PORT, &GPIO_InitStructure);
    
    // 3. 配置 TIM3 (触发源) - 产生 10kHz 更新事件
    // 系统时钟 72MHz, 预分频 72-1 = 1MHz时钟 (1us)
    // 自动重装载值 = (1000000 / 10000) - 1 = 99 (即100us周期)
    TIM_TimeBaseStructure.TIM_Period = (1000000 / OPT101_SAMPLE_RATE_HZ) - 1; 
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; 
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    // 选择 TIM3 的 Update 事件作为 TRGO 输出
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update); 
    
    // 4. 配置 ADC1
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 72M/6 = 12MHz ADC时钟
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // 必须关，由TIM触发单次转换
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO; // TIM3触发
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    // 采样时间配置：13.5周期 + 12.5周期 = 26周期，约 2.16us，对100us采样周期绰绰有余
    ADC_RegularChannelConfig(ADC1, OPT101_ADC_CH, 1, ADC_SampleTime_13Cycles5); 
    
    // 开启 ADC 外部触发和 DMA 传输
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);
    ADC_DMACmd(ADC1, ENABLE);
    
    // 5. 配置 DMA1 通道1
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)opt101_adc_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = OPT101_BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // 单次模式（存满2000个停止）
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    
    // 6. ADC 内部校准（加入超时机制）
    ADC_Cmd(ADC1, ENABLE);
    
    
    // 重置校准
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1))
    {
        if(timeout-- == 0) return 1; // 校准重置失败
    }
    
    // 开始校准
    ADC_StartCalibration(ADC1);
    timeout = 0x000FFFFF;
    while(ADC_GetCalibrationStatus(ADC1))
    {
        if(timeout-- == 0) return 1; // 校准过程失败
    }
    
    return 0; // 全部配置及校准通过，返回成功
}

/********************* 启动一次示波器级数据捕获 *********************/
void OPT101_StartCapture(void)
{
    // 关定时器和DMA，重置状态
    TIM_Cmd(TIM3, DISABLE);
    DMA_Cmd(DMA1_Channel1, DISABLE);
    
    // 重新设置 DMA 搬运数量
    DMA_SetCurrDataCounter(DMA1_Channel1, OPT101_BUFFER_SIZE);
    DMA_ClearFlag(DMA1_FLAG_TC1);
    
    // 启动 DMA 和 定时器
    DMA_Cmd(DMA1_Channel1, ENABLE);
    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);
}

/********************* 检查是否捕获完成 *********************/
uint8_t OPT101_IsCaptureDone(void)
{
    if(DMA_GetFlagStatus(DMA1_FLAG_TC1)) // 传输完成标志位
    {
        TIM_Cmd(TIM3, DISABLE); // 停止采样
        DMA_Cmd(DMA1_Channel1, DISABLE);
        DMA_ClearFlag(DMA1_FLAG_TC1);
        return 1;
    }
    return 0;
}

/********************* 核心算法：分析 10%-90% 响应时间 *********************/
OPT101_ResultTypeDef OPT101_AnalyzeBuffer(void)
{
    OPT101_ResultTypeDef result = {0};
    uint16_t min_val = 4095, max_val = 0;
    uint32_t start_avg = 0, end_avg = 0;
    uint16_t th_10, th_90;
    int idx_10 = -1, idx_90 = -1;
    int i;
    
    // 1. 遍历找出最大值和最小值
    for(i = 0; i < OPT101_BUFFER_SIZE; i++) {
        if(opt101_adc_buffer[i] > max_val) max_val = opt101_adc_buffer[i];
        if(opt101_adc_buffer[i] < min_val) min_val = opt101_adc_buffer[i];
    }
    
    result.v_min = min_val;
    result.v_max = max_val;
    
    // 2. 检查是否有有效跳变（设定一个阈值，例如电压差小于 400 个ADC值即 0.3V 认为没变化）
    if((max_val - min_val) < 400) {
        result.valid = 0;
        return result; // 波动太小，可能是环境噪声
    }
    result.valid = 1;
    
    // 3. 判断是上升沿还是下降沿（比较前20个点和后20个点的平均值）
    for(i = 0; i < 20; i++) {
        start_avg += opt101_adc_buffer[i];
        end_avg += opt101_adc_buffer[OPT101_BUFFER_SIZE - 1 - i];
    }
    start_avg /= 20;
    end_avg /= 20;
    
    result.is_rising = (end_avg > start_avg) ? 1 : 0;
    
    // 4. 计算 10% 和 90% 的阈值
    th_10 = (uint16_t)(min_val + (max_val - min_val) * 0.1f);
    th_90 = (uint16_t)(min_val + (max_val - min_val) * 0.9f);
    
    // 5. 寻找 10% 和 90% 穿越点
    if(result.is_rising) {
        // 上升沿寻找
        for(i = 0; i < OPT101_BUFFER_SIZE; i++) {
            if(idx_10 == -1 && opt101_adc_buffer[i] >= th_10) idx_10 = i;
            if(idx_10 != -1 && idx_90 == -1 && opt101_adc_buffer[i] >= th_90) {
                idx_90 = i;
                break;
            }
        }
    } else {
        // 下降沿寻找 (注意此时是从高往低掉，先遇到90%，再遇到10%)
        // 我们计算时间依然用 T(10%) - T(90%)，所以先找跌破90%的点
        for(i = 0; i < OPT101_BUFFER_SIZE; i++) {
            if(idx_90 == -1 && opt101_adc_buffer[i] <= th_90) idx_90 = i;
            if(idx_90 != -1 && idx_10 == -1 && opt101_adc_buffer[i] <= th_10) {
                idx_10 = i;
                break;
            }
        }
    }
    
    // 6. 计算时间差
    if(idx_10 != -1 && idx_90 != -1) {
        // 索引差值 * 采样周期(秒) * 1000 = 毫秒
        int delta_idx = (result.is_rising) ? (idx_90 - idx_10) : (idx_10 - idx_90);
        result.time_ms = (float)delta_idx * (1.0f / OPT101_SAMPLE_RATE_HZ) * 1000.0f;
    } else {
        result.valid = 0; // 虽然振幅够，但是没有找到完整的穿越过程
    }
    
    return result;
}

#include "opt3001.h"
#include "delay.h"  

/********************* 微秒级延时封装（加static消除原型警告） *********************/
static void OPT3001_DelayUs(u32 us)
{
    Delay_us(us); // 调用通用延时函数
}

/********************* IIC底层驱动实现 *********************/
// IIC引脚初始化（推挽输出，软件控制电平）
void OPT3001_IIC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    
    // 使能GPIO时钟
    RCC_APB2PeriphClockCmd(OPT3001_IIC_RCC, ENABLE);
    
    // 配置SCL和SDA引脚为推挽输出
    GPIO_InitStruct.GPIO_Pin = OPT3001_IIC_SCL_PIN | OPT3001_IIC_SDA_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OPT3001_IIC_PORT, &GPIO_InitStruct);
    
    // 初始化IIC总线为高电平
    IIC_SCL_HIGH();
    IIC_SDA_HIGH();
}

// IIC起始信号：SCL高电平时，SDA由高变低
void OPT3001_IIC_Start(void)
{
    IIC_SDA_HIGH();
    IIC_SCL_HIGH();
    OPT3001_DelayUs(2);
    IIC_SDA_LOW();
    OPT3001_DelayUs(2);
    IIC_SCL_LOW();  // 拉低SCL，准备发送/接收数据
}

// IIC停止信号：SCL高电平时，SDA由低变高
void OPT3001_IIC_Stop(void)
{
    IIC_SDA_LOW();
    IIC_SCL_HIGH();
    OPT3001_DelayUs(2);
    IIC_SDA_HIGH();
    OPT3001_DelayUs(2);
}

// IIC等待应答（返回1：无应答，0：有应答）
u8 OPT3001_IIC_WaitAck(void)
{
    u8 timeout = 0;
    
    IIC_SDA_HIGH();  // 释放SDA，由从机控制
    OPT3001_DelayUs(1);
    IIC_SCL_HIGH();
    OPT3001_DelayUs(1);
    
    // 等待从机拉低SDA
    while(IIC_SDA_READ())
    {
        timeout++;
        if(timeout > 250)
        {
            OPT3001_IIC_Stop();
            return 1;  // 超时无应答
        }
    }
    
    IIC_SCL_LOW();  // 拉低SCL，结束应答检测
    return 0;       // 有应答
}

// IIC发送应答（ack=0：发送应答，ack=1：发送非应答）
void OPT3001_IIC_SendAck(u8 ack)
{
    IIC_SCL_LOW();
    if(ack)
        IIC_SDA_HIGH();  // 非应答
    else
        IIC_SDA_LOW();   // 应答
    OPT3001_DelayUs(1);
    IIC_SCL_HIGH();      // 高电平期间，从机读取应答
    OPT3001_DelayUs(1);
    IIC_SCL_LOW();       // 拉低SCL
    IIC_SDA_HIGH();      // 释放SDA
}

// IIC发送一个字节
void OPT3001_IIC_SendByte(u8 byte)
{
    u8 i;
    
    IIC_SCL_LOW();  // 拉低SCL，准备发送
    for(i=0; i<8; i++)
    {
        // 发送最高位
        if(byte & 0x80)
            IIC_SDA_HIGH();
        else
            IIC_SDA_LOW();
        byte <<= 1;
        OPT3001_DelayUs(1);
        IIC_SCL_HIGH();  // 高电平期间，从机读取数据
        OPT3001_DelayUs(1);
        IIC_SCL_LOW();   // 拉低SCL，准备下一位
    }
    OPT3001_DelayUs(1);
}

// IIC接收一个字节（ack=0：发送应答，ack=1：发送非应答）
u8 OPT3001_IIC_ReceiveByte(u8 ack)
{
    u8 i, byte = 0;
    
    IIC_SDA_HIGH();  // 释放SDA，由从机控制
    for(i=0; i<8; i++)
    {
        IIC_SCL_LOW();
        OPT3001_DelayUs(1);
        IIC_SCL_HIGH();  // 高电平期间，读取SDA电平
        byte <<= 1;
        if(IIC_SDA_READ())
            byte |= 0x01;
        OPT3001_DelayUs(1);
    }
    OPT3001_IIC_SendAck(ack);  // 发送应答
    return byte;
}

/********************* OPT3001传感器驱动实现 *********************/
// 写OPT3001寄存器（16位数据）
u8 OPT3001_WriteReg(u8 reg_addr, u16 data)
{
    OPT3001_IIC_Start();
    // 发送从机地址+写命令（0x44<<1 | 0 = 0x88）
    OPT3001_IIC_SendByte(OPT3001_ADDR << 1);
    if(OPT3001_IIC_WaitAck())  // 等待从机应答
        return 1;
    
    // 发送寄存器地址
    OPT3001_IIC_SendByte(reg_addr);
    if(OPT3001_IIC_WaitAck())
        return 1;
    
    // 发送高8位数据
    OPT3001_IIC_SendByte((data >> 8) & 0xFF);
    if(OPT3001_IIC_WaitAck())
        return 1;
    
    // 发送低8位数据
    OPT3001_IIC_SendByte(data & 0xFF);
    if(OPT3001_IIC_WaitAck())
        return 1;
    
    OPT3001_IIC_Stop();
    return 0;  // 写入成功
}

// 读OPT3001寄存器（16位数据）
u16 OPT3001_ReadReg(u8 reg_addr)
{
    u16 data = 0;
    
    OPT3001_IIC_Start();
    // 发送从机地址+写命令
    OPT3001_IIC_SendByte(OPT3001_ADDR << 1);
    if(OPT3001_IIC_WaitAck())
        return 0xFFFF;  // 读取失败
    
    // 发送寄存器地址
    OPT3001_IIC_SendByte(reg_addr);
    if(OPT3001_IIC_WaitAck())
        return 0xFFFF;
    
    // 重复起始信号
    OPT3001_IIC_Start();
    // 发送从机地址+读命令（0x44<<1 | 1 = 0x89）
    OPT3001_IIC_SendByte((OPT3001_ADDR << 1) | 0x01);
    if(OPT3001_IIC_WaitAck())
        return 0xFFFF;
    
    // 接收高8位（发送应答）- 修复函数名笔误
    data = OPT3001_IIC_ReceiveByte(0) << 8;
    // 接收低8位（发送非应答）
    data |= OPT3001_IIC_ReceiveByte(1);
    
    OPT3001_IIC_Stop();
    return data;
}

// OPT3001初始化（连续转换模式，100ms转换一次）
u8 OPT3001_Init(void)
{
    u16 config = 0;
    
    // 初始化IIC总线
    OPT3001_IIC_Init();
    
    // 配置寄存器：
    // - 12-13位：MODE=11（连续转换模式）
    // - 9-11位：CONV_TIME=100（转换时间100ms）
    // - 其余位：默认值
    config = 0xCE00;
    
    // 写入配置寄存器
    if(OPT3001_WriteReg(OPT3001_CONFIG_REG, config) != 0)
        return 1;  // 初始化失败
    
    // 验证配置是否写入成功
    if(OPT3001_ReadReg(OPT3001_CONFIG_REG) != config)
        return 1;
    
    return 0;  // 初始化成功
}

// 读取光照强度（单位：lux）
float OPT3001_ReadLux(void)
{
    u16 raw_data;
    u8 exponent;
    u16 mantissa;
    float lux;
    
    // 读取结果寄存器
    raw_data = OPT3001_ReadReg(OPT3001_RESULT_REG);
    if(raw_data == 0xFFFF)
        return -1.0f;  // 读取失败
    
    // 解析原始数据：高4位=指数，低12位=尾数
    exponent = (raw_data >> 12) & 0x0F;
    mantissa = raw_data & 0x0FFF;
    
    // 计算光照值（显式类型转换，消除警告）
    lux = (float)mantissa * (float)(1 << exponent) * 0.01f;
    
    return lux;
}


/********************* 异常处理全局变量（静态，仅本文件使用） *********************/
static OPT3001_StatusTypeDef opt3001_status = OPT3001_STATUS_NORMAL; // 传感器状态
static float last_valid_lux = 0.0f;                                // 上一次有效值
static float filter_window[FILTER_WINDOW_SIZE] = {0};               // 滑动窗口缓存
static u8 window_index = 0;                                        // 窗口索引

/********************* 辅助函数：滑动窗口均值滤波 ********************
static float OPT3001_SlidingAvgFilter(float new_val)
{
    // 1. 将新值存入窗口，覆盖最旧的值
    filter_window[window_index] = new_val;
    // 2. 更新索引（循环覆盖）
    window_index = (window_index + 1) % FILTER_WINDOW_SIZE;
    // 3. 计算窗口内所有值的平均值
    float sum = 0.0f;
    for(u8 i=0; i<FILTER_WINDOW_SIZE; i++)
    {
        sum += filter_window[i];
    }
    return sum / FILTER_WINDOW_SIZE;
}
*/

// 辅助函数：滑动窗口中值滤波（窗口大小建议奇数：3/5）
static float OPT3001_SlidingMedianFilter(float new_val)
{
	  float temp[FILTER_WINDOW_SIZE];
		u8 i,j;
	
    // 1. 存入新值
    filter_window[window_index] = new_val;
    window_index = (window_index + 1) % FILTER_WINDOW_SIZE;

    // 2. 复制窗口数据并排序

    for(i=0; i<FILTER_WINDOW_SIZE; i++) temp[i] = filter_window[i];
    // 简单冒泡排序
    for(i=0; i<FILTER_WINDOW_SIZE-1; i++)
    {
        for(j=0; j<FILTER_WINDOW_SIZE-1-i; j++)
        {
            if(temp[j] > temp[j+1])
            {
                float t = temp[j];
                temp[j] = temp[j+1];
                temp[j+1] = t;
            }
        }
    }

    // 3. 返回中间值（中值）
    return temp[FILTER_WINDOW_SIZE/2];
}
/********************* 带异常处理的光照值读取函数 *********************/
float OPT3001_ReadLux_WithFilter(void)
{
    float raw_lux = -1.0f;
    u8 retry_cnt = 0;
    float current_lux = 0.0f;

    // ========== 步骤1：通信异常处理（重试读取） ==========
    while(retry_cnt < OPT3001_MAX_RETRY)
    {
        raw_lux = OPT3001_ReadLux(); // 调用原读取函数
        if(raw_lux != -1.0f) break;  // 读取成功则退出重试
        retry_cnt++;
        Delay_ms(10);                // 重试间隔（避免频繁读取）
    }
    if(raw_lux == -1.0f)
    {
        opt3001_status = OPT3001_STATUS_COMM_ERR;
        return last_valid_lux;       // 沿用上次有效值，避免数据中断
    }

    // ========== 步骤2：量程异常处理 ==========
    if(raw_lux < OPT3001_MIN_VAL || raw_lux > OPT3001_MAX_VAL)
    {
        opt3001_status = OPT3001_STATUS_RANGE_ERR;
        return last_valid_lux;
    }

    // ========== 步骤3：跳变异常处理（限幅） ==========
    if(last_valid_lux != 0.0f) // 非第一次读取时才判断跳变
    {
        float diff = raw_lux - last_valid_lux;
        if(diff > OPT3001_JUMP_THRESH || diff < -OPT3001_JUMP_THRESH)
        {
            opt3001_status = OPT3001_STATUS_JUMP_ERR;
            return last_valid_lux;
        }
    }

    // ========== 步骤4：噪声滤波（滑动均值） ==========
    //current_lux = OPT3001_SlidingAvgFilter(raw_lux);
		current_lux = OPT3001_SlidingMedianFilter(raw_lux);

    // ========== 步骤5：更新状态和历史值 ==========
    opt3001_status = OPT3001_STATUS_NORMAL;
    last_valid_lux = current_lux; // 保存本次有效值

    return current_lux;
}

/********************* 获取传感器状态（用于故障排查） *********************/
OPT3001_StatusTypeDef OPT3001_GetStatus(void)
{
    return opt3001_status;
}


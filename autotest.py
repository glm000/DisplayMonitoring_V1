import serial
import time
import tkinter as tk
import matplotlib.pyplot as plt

# ================= 1. 串口配置 =================
SERIAL_PORT = 'COM5'
BAUD_RATE = 115200

try:
    print(f">> 正在连接串口 {SERIAL_PORT}...")
    ser = serial.Serial()
    ser.port = SERIAL_PORT
    ser.baudrate = BAUD_RATE
    ser.timeout = 2
    ser.dtr = False
    ser.rts = False
    ser.open()
    time.sleep(2)  # 等待 STM32 开机就绪
    ser.reset_input_buffer()
    print(">> 串口已连接！\n")
except Exception as e:
    print(f"串口打开失败: {e}")
    exit()

# ================= 2. 核心控制与数据收集逻辑 =================


def run_test():
    print("="*50)
    print("--- 启动单次【白 -> 黑】(下降沿) 波形抓取 ---")

    # 1. 初始状态：全白（G255），给光电传感器和运放充足的时间达到饱和稳态
    canvas.itemconfig(rect, fill="white")
    root.update()
    time.sleep(1.0)

    ser.reset_input_buffer()

    # 2. 通知 STM32 开启 1 秒高频视窗
    ser.write(b'R\r\n')
    print(">> [ T=0.00s ] 已发送指令 'R'，快门已打开...")

    # 3. 盲区延迟 50ms（让波形开头保留一段平稳的白色高电平）
    time.sleep(0.05)

    # 4. 瞬间切黑 (白 -> 黑跳变)
    canvas.itemconfig(rect, fill="black")
    root.update()
    print(">> [ T=0.05s ] 屏幕已瞬间切黑！")

    print(">> 正在等待 STM32 计算并传输 10000 个数据点 (约需 5~6 秒)...")

    raw_data = []
    is_recording = False

    # 5. 循环读取串口数据
    while True:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
        except:
            continue

        if not line:
            continue

        # 打印 STM32 算出来的结果
        if "响应时间" in line or "稳态" in line or "跳变方向" in line:
            print(f"   [STM32] {line}")

        # 侦测数据流起始和结束标志
        if "--- RAW_DATA_START ---" in line:
            is_recording = True
            print(">> 开始接收高频波形数据，请稍候...")
            continue
        elif "--- RAW_DATA_END ---" in line:
            print(f">> 数据传输完毕！成功接收 {len(raw_data)} 个点。")
            break

        # 记录原始数据
        if is_recording:
            try:
                raw_data.append(int(line))
            except ValueError:
                pass  # 过滤掉可能的乱码杂质

    # 6. 数据收集完毕，关闭全屏窗口，开始画图
    root.destroy()
    plot_waveform(raw_data)

# ================= 3. 波形绘图引擎 =================


def plot_waveform(data):
    if len(data) < 100:
        print("未收到足够的波形数据！")
        return

    # 将 ADC 值 (0-4095) 换算成电压值 (0-3.3V)
    voltages = [(val / 4095.0) * 3.3 for val in data]

    # 构建时间轴 (10000个点，每个点相距 0.1ms)
    times = [i * 0.1 for i in range(len(data))]

    # 估算波形的峰值和谷值 (取前100个点和后100个点的均值)
    v_max_avg = sum(voltages[:100]) / 100
    v_min_avg = sum(voltages[-100:]) / 100

    # 计算 10% 和 90% 的阈值电压
    v90 = v_min_avg + (v_max_avg - v_min_avg) * 0.9
    v10 = v_min_avg + (v_max_avg - v_min_avg) * 0.1

    # 初始化图表 (使用英文标签避免乱码)
    plt.figure(figsize=(12, 6))

    # 画出主波形
    plt.plot(times, voltages, color='#007acc',
             linewidth=1.5, label='OPT101 Signal')

    # 画出 10% 和 90% 的基准线
    plt.axhline(y=v90, color='red', linestyle='--',
                alpha=0.7, label='90% Threshold')
    plt.axhline(y=v10, color='green', linestyle='--',
                alpha=0.7, label='10% Threshold')

    plt.title('Display Fall Time ($T_f$) Waveform: White -> Black',
              fontsize=14, fontweight='bold')
    plt.xlabel('Time (ms)', fontsize=12)
    plt.ylabel('Sensor Output Voltage (V)', fontsize=12)
    plt.grid(True, which='both', linestyle=':', alpha=0.6)
    plt.legend(loc='upper right')

    # 限制 X 轴显示范围 (为了看清跳变细节，只显示前 300 毫秒)
    plt.xlim(0, 300)

    plt.tight_layout()
    print(">> 正在渲染波形图...")
    plt.show()


# ================= 4. UI 启动 =================
root = tk.Tk()
root.attributes("-fullscreen", True)
root.config(cursor="none")
canvas = tk.Canvas(root, highlightthickness=0)
canvas.pack(fill="both", expand=True)
rect = canvas.create_rectangle(0, 0, root.winfo_screenwidth(
), root.winfo_screenheight(), fill="white")  # 初始为白

print(">> UI 已就绪。请将 OPT101 传感器紧贴屏幕！")
print(">> 3 秒后将自动进行【白 -> 黑】跳变测试并生成图表...")

root.after(3000, run_test)
root.mainloop()

ser.close()

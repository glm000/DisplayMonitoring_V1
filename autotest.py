import tkinter as tk
import serial
import threading
import time

# ================= 配置区 =================
COM_PORT = 'COM5'         # 请修改为你实际连接单片机的串口号
BAUD_RATE = 115200        # 与单片机保持一致
SWITCH_INTERVAL_MS = 2000  # 自动切换的时间间隔（毫秒），2000ms=2秒
# ==========================================

# 尝试连接串口
try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.5)
    print(f"成功连接到 {COM_PORT} @ {BAUD_RATE} bps")
except Exception as e:
    print(f"串口打开失败，请检查端口是否被占用: {e}")
    ser = None

# 后台线程：持续读取单片机返回的分析结果


def read_serial():
    while True:
        if ser and ser.in_waiting:
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"[STM32] {line}")
            except Exception:
                pass
        time.sleep(0.01)


if ser:
    # 启动后台读取线程
    t = threading.Thread(target=read_serial, daemon=True)
    t.start()

is_auto_running = False  # 记录当前是否在自动运行

# 核心自动切换动作


def perform_switch():
    if not is_auto_running:
        return  # 如果已经暂停，则停止循环

    # 获取当前屏幕颜色，决定下一次的颜色
    current_color = root.cget("bg")
    next_color = "white" if current_color == "black" else "black"

    if ser:
        ser.reset_input_buffer()
        # 1. 瞬间发送抓捕指令
        ser.write(b'R')

    # 2. 瞬间切换屏幕颜色 (紧跟在串口发送之后)
    root.config(bg=next_color)
    root.update()  # 强制立刻刷新屏幕

    direction = "黑 -> 白 (上升沿)" if next_color == "white" else "白 -> 黑 (下降沿)"
    print(f"\n>> [自动测试] 触发抓取: {direction}")

    # 3. 安排下一次自动切换
    root.after(SWITCH_INTERVAL_MS, perform_switch)

# 启动/暂停控制


def toggle_auto_mode(event=None):
    global is_auto_running
    if not is_auto_running:
        is_auto_running = True
        print(">> 开始自动测试循环...")
        perform_switch()  # 立即执行一次并开启循环
    else:
        is_auto_running = False
        print(">> 已暂停自动测试。再次按下【空格键】恢复。")

# 退出程序


def exit_app(event=None):
    global is_auto_running
    is_auto_running = False  # 停止循环
    if ser:
        ser.close()
    root.destroy()


# ================= GUI 初始化 =================
root = tk.Tk()
# 设置全屏无边框
root.attributes('-fullscreen', True)
# 隐藏鼠标光标（防止光标影响光照传感器）
root.config(cursor="none")
# 初始颜色设为纯黑
root.config(bg="black")

# 绑定快捷键
root.bind('<space>', toggle_auto_mode)  # 空格键现在变成了 启动/暂停 的开关
root.bind('<Escape>', exit_app)        # 按 Esc 键退出

print("\n===========================================")
print("全自动屏幕测试程序已启动。")
print(f"当前设定的切换间隔为: {SWITCH_INTERVAL_MS} 毫秒")
print("1. 请将 OPT101 传感器紧贴在屏幕中央，并做好遮光。")
print("2. 按下【空格键】启动/暂停自动黑白交替测试。")
print("3. 按下【Esc 键】退出测试。")
print("===========================================\n")

# 进入主循环
root.mainloop()

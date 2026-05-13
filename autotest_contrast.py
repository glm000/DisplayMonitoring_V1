import tkinter as tk
import screen_brightness_control as sbc


class ContrastTestScreen:
    def __init__(self, root, interval_ms=2500):
        self.root = root
        self.interval = interval_ms
        self.is_white = True
        self.original_brightness = None

        # 1. 尝试获取并保存当前屏幕的原始亮度，然后强制拉满到 100%
        try:
            # get_brightness() 返回的是一个列表（因为电脑可能连接了多个显示器）
            self.original_brightness = sbc.get_brightness()
            print(f">>> 记录当前显示器亮度: {self.original_brightness}")

            sbc.set_brightness(100)
            print(">>> 已自动将显示器亮度拉满至 100%")
        except Exception as e:
            print(f">>> [警告] 屏幕亮度控制初始化失败: {e}")
            print(">>> 请手动将显示器亮度调至最高以确保测试精度。")

        # 2. 设置全屏显示，去掉标题栏和边框
        self.root.attributes("-fullscreen", True)

        # 隐藏鼠标指针，防止光标的白色像素干扰黑场测试
        self.root.config(cursor="none")

        # 3. 绑定退出事件 (按 ESC 或 窗口被强行关闭)
        self.root.bind("<Escape>", self.exit_test)
        self.root.protocol("WM_DELETE_WINDOW", self.exit_test)

        # 初始背景色设为纯白
        self.root.configure(bg='white')

        # 启动定时切换任务
        self.schedule_toggle()

    def toggle_color(self):
        # 翻转颜色状态
        self.is_white = not self.is_white

        # 根据状态设置十六进制纯白或纯黑
        new_color = '#FFFFFF' if self.is_white else '#000000'
        self.root.configure(bg=new_color)

        # 安排下一次切换
        self.schedule_toggle()

    def schedule_toggle(self):
        # 使用 tkinter 内置的 after 方法实现非阻塞定时器
        self.root.after(self.interval, self.toggle_color)

    def exit_test(self, event=None):
        print("\n测试结束，准备退出全屏...")

        # 4. 退出前恢复原始屏幕亮度
        if self.original_brightness is not None:
            try:
                # 针对多显示器情况，将亮度恢复为原先的列表状态
                for i, brightness_val in enumerate(self.original_brightness):
                    sbc.set_brightness(brightness_val, display=i)
                print(f">>> 已成功恢复原始屏幕亮度: {self.original_brightness}")
            except Exception as e:
                print(f">>> [错误] 恢复屏幕亮度失败: {e}")

        self.root.destroy()


if __name__ == "__main__":
    print("=======================================")
    print("自动对比度测试 (带动态亮度控制) 已启动")
    print("屏幕将在黑白之间每隔 2.5 秒自动切换")
    print("提示：按 [ESC] 键可退出测试并恢复亮度")
    print("=======================================")

    root = tk.Tk()
    app = ContrastTestScreen(root, interval_ms=2500)
    root.mainloop()

import cv2
import numpy as np
import time


class ScreenMonitor:
    def __init__(self, black_threshold=10, freeze_threshold=1.0):
        """
        初始化监测器
        :param black_threshold: 黑屏判定阈值
        :param freeze_threshold: 卡死/触控判定阈值
        """
        self.black_threshold = black_threshold
        self.freeze_threshold = freeze_threshold
        self.last_frame = None  # 用来存储上一帧，用于对比卡死

    def check_black_screen(self, image):
        """检测当前帧是否黑屏"""
        if image is None:
            return False, "图像为空"

        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        avg_val = np.mean(gray)

        if avg_val < self.black_threshold:
            return True, f"检测到黑屏 (亮度: {avg_val:.2f})"
        return False, "屏幕正常"

    def check_freeze(self, current_image):
        """检测画面是否相对于上一帧卡死"""
        if self.last_frame is None:
            # 如果是第一次运行，没有上一帧，就先存下来，跳过检测
            self.last_frame = current_image
            return False, "初始化帧 (无对比数据)"

        # 1. 转灰度
        gray1 = cv2.cvtColor(self.last_frame, cv2.COLOR_BGR2GRAY)
        gray2 = cv2.cvtColor(current_image, cv2.COLOR_BGR2GRAY)

        # 2. 确保尺寸一致
        if gray1.shape != gray2.shape:
            # 如果尺寸变了，重置上一帧
            self.last_frame = current_image
            return False, "分辨率改变，重置对比帧"

        # 3. 计算差异
        diff = cv2.absdiff(gray1, gray2)
        score = np.mean(diff)

        # 更新上一帧 (为下一次检测做准备)
        self.last_frame = current_image

        if score < self.freeze_threshold:
            return True, f"检测到画面卡死 (变化度: {score:.4f})"
        else:
            return False, f"画面正常运行 (变化度: {score:.4f})"

    def verify_touch(self, image_before, image_after):
        """验证触控动作 (对比点击前后)"""
        # 复用上面的卡死检测逻辑，但含义相反
        # 触控成功 = 画面有变化 (score > threshold)

        gray1 = cv2.cvtColor(image_before, cv2.COLOR_BGR2GRAY)
        gray2 = cv2.cvtColor(image_after, cv2.COLOR_BGR2GRAY)

        if gray1.shape != gray2.shape:
            return False, "尺寸不一致"

        diff = cv2.absdiff(gray1, gray2)
        score = np.mean(diff)

        if score > self.freeze_threshold:
            return True, f"触控成功 (变化度: {score:.4f})"
        else:
            return False, f"触控失效/无响应 (变化度: {score:.4f})"


# --- 主程序运行逻辑 (模拟) ---
if __name__ == "__main__":
    # 1. 启动监测员
    monitor = ScreenMonitor(black_threshold=10, freeze_threshold=1.0)

    print("🚀 屏幕监控系统已启动...\n")

    # 模拟读取到的图片序列 (你可以换成摄像头 cap.read())
    # 假设：第一秒正常，第二秒黑屏，第三秒卡死
    test_files = ['test_b.jpg', 'test_b.jpg', 'test_w.jpg']

    # 2. 模拟循环检测
    for i, file_name in enumerate(test_files):
        print(f"--- 第 {i+1} 次检测 ({file_name}) ---")

        # 读取当前帧
        frame = cv2.imread(file_name)
        if frame is None:
            print(f"❌ 无法读取图片: {file_name}")
            continue

        # [步骤 A] 先查黑屏
        is_black, msg_black = monitor.check_black_screen(frame)
        if is_black:
            print(f"🚨 严重故障: {msg_black}")
            # 如果黑屏了，通常就不需要测卡死了，直接进入下一轮
            continue

        # [步骤 B] 再查卡死 (需要和上一次的图片对比)
        is_frozen, msg_freeze = monitor.check_freeze(frame)
        if is_frozen:
            print(f"⚠️ 警告: {msg_freeze}")
        else:
            print(f"✅ {msg_freeze}")

        time.sleep(1)  # 模拟间隔

    # [步骤 C] 单独测试触控 (当机器臂执行点击动作时调用)
    print("\n--- 触发测试: 点击动作验证 ---")
    # 传入两张图：点击前，点击后
    success, msg_touch = monitor.verify_touch(
        cv2.imread('test_b.jpg'), cv2.imread('test_w.jpg'))
    print(f"👉 触控测试结果: {msg_touch}")

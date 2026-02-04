import cv2
import numpy as np


def check_black_screen(image_path, threshold=10):
    """
    检测指定图片是否为黑屏
    :param image_path: 图片路径
    :param threshold: 判定阈值（越小越严格，0为纯黑）
    """
    # 1. 读取图像 (OpenCV 读取进来的是 BGR 格式的矩阵)
    img = cv2.imread(image_path)

    # 安全检查：防止路径错误导致读不到图片
    if img is None:
        print(f"❌ 错误：无法找到图片，请检查路径: {image_path}")
        return

    # 2. 转为灰度图
    # 这一步把彩色的三通道 (Blue, Green, Red) 变成单通道的“亮度图”
    gray_img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    # 3. 计算平均亮度 (核心算法)
    avg_val = np.mean(gray_img)

    print(f"📊 当前图片平均亮度: {avg_val:.2f} (阈值: {threshold})")

    # 4. 逻辑判定
    if avg_val < threshold:
        print("✅ 检测结果：是黑屏")
    else:
        print("💡 检测结果：屏幕有点亮，不是黑屏")


# --- 测试部分 ---
# 请确保你的代码目录下有一张名为 test.jpg 的图片，或者修改下面的路径
check_black_screen('test_w.jpg')

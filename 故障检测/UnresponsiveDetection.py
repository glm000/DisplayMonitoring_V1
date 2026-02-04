import cv2
import numpy as np


def check_freeze(image1_path, image2_path, threshold=1.0):
    """
    检测画面是否卡死（对比两张图片）
    :param image1_path: t1 时刻的图片路径
    :param image2_path: t2 时刻的图片路径
    :param threshold: 判断阈值（越小越灵敏，建议 1.0~5.0 之间）
    """
    # 1. 读取两张图片
    img1 = cv2.imread(image1_path)
    img2 = cv2.imread(image2_path)

    # 安全检查
    if img1 is None or img2 is None:
        print("❌ 错误：无法读取图片，请检查路径。")
        return

    # 2. 转为灰度图 (减少计算量，排除颜色干扰)
    gray1 = cv2.cvtColor(img1, cv2.COLOR_BGR2GRAY)
    gray2 = cv2.cvtColor(img2, cv2.COLOR_BGR2GRAY)

    # 3. 计算差异 (核心算法: 帧差法)
    # 得到一张“差异图”，不一样的地方是亮的，一样的地方是黑的
    diff_img = cv2.absdiff(gray1, gray2)

    # 4. 量化差异 (计算平均值)
    change_score = np.mean(diff_img)

    print(f"📉 画面变化程度 (Score): {change_score:.4f} (阈值: {threshold})")

    # 5. 逻辑判定
    if change_score < threshold:
        print("⚠️ 检测结果：画面卡死 (无响应)")
    else:
        print("✅ 检测结果：画面正常 (在变化)")

# --- 测试部分 ---
# 你需要准备两张图片：
# Case A (模拟卡死): 复制 test.jpg 为 test_copy.jpg，两张图完全一样。
# Case B (正常): 找两张不同的图。


print("--- 测试 1: 两张完全一样的图 ---")
check_freeze('test_w.jpg', 'test_w.jpg')  # 自己和自己比，绝对是卡死

# print("\n--- 测试 2: 两张不同的图 (如果有的话) ---")
# check_freeze('test_b.jpg', 'test_w.jpg')

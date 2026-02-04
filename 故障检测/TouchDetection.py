import cv2
import numpy as np


def verify_touch_action(before_img_path, after_img_path, threshold=1.0):
    """
    验证触控动作是否生效
    :param before_img_path: 点击前的截图
    :param after_img_path: 点击后的截图
    """
    # 1. 读取图片
    img_before = cv2.imread(before_img_path)
    img_after = cv2.imread(after_img_path)

    if img_before is None or img_after is None:
        print("❌ 错误：无法读取图片")
        return

    # 2. 转灰度 & 3. 帧差法
    gray_before = cv2.cvtColor(img_before, cv2.COLOR_BGR2GRAY)
    gray_after = cv2.cvtColor(img_after, cv2.COLOR_BGR2GRAY)

    # 注意：必须确保两张图尺寸一致，否则 absdiff 会报错
    if gray_before.shape != gray_after.shape:
        print("❌ 错误：两张图片尺寸不一致，无法对比")
        return

    diff = cv2.absdiff(gray_before, gray_after)
    change_score = np.mean(diff)

    print(f"📊 动作响应度: {change_score:.4f} (阈值: {threshold})")

    # 4. 逻辑判定 (这里逻辑是反过来的：没有变化才是故障)
    if change_score > threshold:
        print("✅ 测试通过：检测到屏幕内容已更新 (触控成功)")
    else:
        print("❌ 测试失败：屏幕无变化 (触控失效)")


# --- 模拟测试 ---
# 场景 1: 触控失效 (画面没变)
print("--- 测试场景 1: 触控失效 ---")
verify_touch_action('test_b.jpg', 'test_b.jpg')

# 场景 2: 触控成功 (画面变了) - 这里你需要两张不同的图来测试
# verify_touch_action('test_b.jpg', 'test_w.jpg')

from PIL import Image
import numpy as np
import cv2

# -------------------------------
# 配置区域（RGB方式）
TARGET_COLORS = [
    (237, 173, 57),   # 上层黄
    (160, 87, 34),    # 下层棕
]
COLOR_TOLERANCE = 30                # RGB差异容忍范围
MIN_AREA = 2000                     # 最小识别面积
PIXEL_SCALE = 1.0
# -------------------------------

def color_close(c1, c2, tolerance):
    return all(abs(a - b) <= tolerance for a, b in zip(c1, c2))

def extract_platforms_rgb(image_path, show_preview=True):
    image = Image.open(image_path).convert("RGB")
    img_np = np.array(image)

    # 构造掩码图（将平台色区域设为255，其余设为0）
    mask = np.zeros((img_np.shape[0], img_np.shape[1]), dtype=np.uint8)
    for color in TARGET_COLORS:
        color_mask = np.all(np.abs(img_np - color) <= COLOR_TOLERANCE, axis=-1)
        mask[color_mask] = 255

    # 闭运算填补缝隙
    kernel = np.ones((3, 3), np.uint8)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel, iterations=1)

    # 连通区域检测
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    rects = []
    for contour in contours:
        x, y, w, h = cv2.boundingRect(contour)
        if w * h >= MIN_AREA:
            rects.append((int(x * PIXEL_SCALE), int(y * PIXEL_SCALE),
                          int(w * PIXEL_SCALE), int(h * PIXEL_SCALE)))

    # 输出 Qt 格式
    print("QVector<QRect> platformRects = {")
    for x, y, w, h in rects:
        print(f"    QRect({x}, {y}, {w}, {h}),")
    print("};")

    if show_preview:
        preview = img_np.copy()
        for x, y, w, h in rects:
            cv2.rectangle(preview, (x, y), (x + w, y + h), (255, 0, 0), 2)
        cv2.imshow("Detected Platforms", cv2.cvtColor(preview, cv2.COLOR_RGB2BGR))
        cv2.waitKey(0)
        cv2.destroyAllWindows()

if __name__ == "__main__":
    extract_platforms_rgb("default_map.png", show_preview=True)

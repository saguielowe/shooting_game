from PIL import Image
import numpy as np
import cv2

# -------------------------------
# 配置区域
LOWER_BLUE = np.array([90,  40, 40])     # HSV蓝色下界
UPPER_BLUE = np.array([140, 255, 255])   # HSV蓝色上界
MIN_AREA = 2000                            # 忽略过小区域
PIXEL_SCALE = 1.0                        # 游戏单位缩放比例
# -------------------------------

def extract_platforms(image_path, show_preview=True):
    # 读取图片
    image = Image.open(image_path).convert("RGB")
    img_np = np.array(image)
    
    # 转 HSV 掩码图
    hsv = cv2.cvtColor(img_np, cv2.COLOR_RGB2HSV)
    mask = cv2.inRange(hsv, LOWER_BLUE, UPPER_BLUE)

    # 膨胀 + 腐蚀，填补间隙
    kernel = np.ones((3, 3), np.uint8)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel, iterations=1)

    # 用轮廓提取出区域
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    rects = []
    for contour in contours:
        # 生成掩码图：仅当前轮廓区域为白，其余为黑
        platform_mask = np.zeros(mask.shape, dtype=np.uint8)
        cv2.drawContours(platform_mask, [contour], -1, 255, thickness=cv2.FILLED)

        # 在这个掩码图上做网格扫描：将所有非0区域切分成最小矩形
        visited = np.zeros_like(platform_mask, dtype=bool)
        h, w = platform_mask.shape
        for y in range(h):
            for x in range(w):
                if platform_mask[y, x] == 255 and not visited[y, x]:
                    # 扫描当前连通块行
                    x0 = x
                    while x < w and platform_mask[y, x] == 255 and not visited[y, x]:
                        visited[y, x] = True
                        x += 1
                    width = x - x0
                    # 扫描高度（纵向向下，保持这一行的横向延展）
                    height = 1
                    while y + height < h and np.all(platform_mask[y + height, x0:x0 + width] == 255) \
                            and not np.any(visited[y + height, x0:x0 + width]):
                        visited[y + height, x0:x0 + width] = True
                        height += 1
                    # 标记区域
                    visited[y:y + height, x0:x0 + width] = True
                    if width * height >= MIN_AREA:
                        rects.append((
                            int(x0 * PIXEL_SCALE),
                            int(y * PIXEL_SCALE),
                            int(width * PIXEL_SCALE),
                            int(height * PIXEL_SCALE)
                        ))

    # 输出 Qt 格式
    print("QVector<QRect> platformRects = {")
    for x, y, w, h in rects:
        print(f"    QRect({x}, {y}, {w}, {h}),")
    print("};")

    # 可视化
    if show_preview:
        preview = img_np.copy()
        for x, y, w, h in rects:
            cv2.rectangle(preview, (x, y), (x + w, y + h), (255, 0, 0), 2)
        cv2.imshow("Detected Platforms", cv2.cvtColor(preview, cv2.COLOR_RGB2BGR))
        cv2.waitKey(0)
        cv2.destroyAllWindows()

if __name__ == "__main__":
    extract_platforms("default_map.png", show_preview=True)

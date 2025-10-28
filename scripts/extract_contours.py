import cv2
import json
import numpy as np # 1. 导入 numpy

def extract_polygon(image_path, epsilon_factor=0.01):
    """
    从图像中提取最主要的轮廓并将其简化为多边形。
    """
    # 1. 加载图像并处理
    image = cv2.imread(image_path)
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    # 阈值处理，将图像变为黑白
    _, thresh = cv2.threshold(gray, 127, 255, cv2.THRESH_BINARY_INV)

    # 2. 查找轮廓
    # 我们只关心最外层的轮廓
    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    if not contours:
        return None

    # 3. 找到最大的轮廓
    main_contour = max(contours, key=cv2.contourArea)

    # 4. 简化轮廓 (Ramer-Douglas-Peucker 算法)
    # 这是关键步骤，减少顶点数量，使其适用于渐变算法
    epsilon = epsilon_factor * cv2.arcLength(main_contour, True)
    approx_polygon = cv2.approxPolyDP(main_contour, epsilon, True)

    # 格式化为 [[x1, y1], [x2, y2], ...]
    vertices = approx_polygon.reshape(-1, 2).tolist()
    return vertices

# --- 使用示例 ---
# 2. 将路径保存为变量，方便重复使用
path_a = "../assets/image_a.png"
path_b = "../assets/image_b.png"
output_path_a = "../assets/poly_a.json"
output_path_b = "../assets/poly_b.json"

target_epsilon_factor = 0.0001

poly_a_vertices = extract_polygon(path_a,target_epsilon_factor)
poly_b_vertices = extract_polygon(path_b,target_epsilon_factor)

# 5. 保存为JSON，供C++程序读取
with open(output_path_a, "w") as f:
    json.dump(poly_a_vertices, f)

with open(output_path_b, "w") as f:
    json.dump(poly_b_vertices, f)

print(f"提取了 {len(poly_a_vertices)} 个顶点到 {output_path_a}")
print(f"提取了 {len(poly_b_vertices)} 个顶点到 {output_path_b}")


# ---------------------------------------------------
# 3. --- 新增的可视化代码 ---
# ---------------------------------------------------
print("\n按任意键关闭预览窗口...")

# 重新加载彩色图像用于显示
img_a_display = cv2.imread(path_a)
img_b_display = cv2.imread(path_b)

# 检查是否成功提取了轮廓
if poly_a_vertices:
    # 将顶点列表 [[x,y], ...] 转换为 (N, 1, 2) 的 numpy 数组
    contour_a_np = np.array(poly_a_vertices, dtype=np.int32).reshape((-1, 1, 2))
    # 在图像上绘制轮廓（绿色，2像素粗细）
    cv2.drawContours(img_a_display, [contour_a_np], -1, (0, 255, 0), 2)
    # 显示图像
    cv2.imshow("Image A - Contour", img_a_display)

if poly_b_vertices:
    # 同上
    contour_b_np = np.array(poly_b_vertices, dtype=np.int32).reshape((-1, 1, 2))
    # 在图像上绘制轮廓（红色，2像素粗细）
    cv2.drawContours(img_b_display, [contour_b_np], -1, (0, 0, 255), 2)
    # 显示图像
    cv2.imshow("Image B - Contour", img_b_display)

# 等待用户按键
cv2.waitKey(0)
# 关闭所有OpenCV窗口
cv2.destroyAllWindows()
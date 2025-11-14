import cv2
import json
import numpy as np
import tkinter as tk
from tkinter import filedialog
import sys

# -----------------------------------------------------------------
# GUI 文件选择器
# -----------------------------------------------------------------
def select_file(title="Select a file"):
    """
    打开一个GUI窗口，让用户从文件夹中选择一个图像文件。
    """
    try:
        root = tk.Tk()
        root.withdraw()  # 隐藏主 Tk 窗口
        root.attributes("-topmost", True)  # 确保文件对话框在最上层
        file_path = filedialog.askopenfilename(
            title=title,
            filetypes=[("Image Files", "*.png *.jpg *.jpeg *.bmp *.webp")]
        )
        root.destroy()
        
        if not file_path:
            print("(!) No file selected. Exiting.")
            sys.exit()
            
        print(f"Selected: {file_path}")
        return file_path
    except Exception as e:
        print(f"(!) GUI Error: {e}. Tkinter (GUI) 库可能未正确安装或配置。")
        print("请确保 'python-tk' (或类似) 包已安装。")
        sys.exit()

# -----------------------------------------------------------------
#  extract_polygon 边缘提取
# -----------------------------------------------------------------
def extract_polygon(image_path, epsilon_factor=0.005):
    """
    从图像中提取最主要的轮廓，并使用一个 *给定* 的 epsilon_factor 将其简化。
    """
    image = cv2.imread(image_path)
    if image is None:
        print(f"(!) Error: Could not read image at {image_path}")
        return None, 0.0

    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    _, thresh = cv2.threshold(gray, 127, 255, cv2.THRESH_BINARY_INV)

    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    if not contours:
        print(f"(!) No contours found in {image_path}")
        return None, 0.0

    main_contour = max(contours, key=cv2.contourArea)
    arc_length = cv2.arcLength(main_contour, True)

    epsilon = epsilon_factor * arc_length
    approx_polygon = cv2.approxPolyDP(main_contour, epsilon, True)
    
    vertices = approx_polygon.reshape(-1, 2).tolist()
    return vertices

# -----------------------------------------------------------------
# 迭代搜索以满足“顶点密度” 
# -----------------------------------------------------------------
def find_polygon_near_target(image_path, target_count):
    """
    自动迭代搜索，找到一个最佳 epsilon_factor，
    使得提取出的顶点数 *接近* target_count。
    """
    print(f"Finding contour for {image_path} (target: ~{target_count} vertices)...")
    image = cv2.imread(image_path)
    if image is None: return None, 0.0
    
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    _, thresh = cv2.threshold(gray, 127, 255, cv2.THRESH_BINARY_INV)
    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not contours: return None, 0.0

    main_contour = max(contours, key=cv2.contourArea)
    arc_length = cv2.arcLength(main_contour, True)

    # 迭代搜索 epsilon_factor
    epsilon_factor = 0.005  # 初始猜测值
    best_polygon = None
    best_epsilon_factor = epsilon_factor

    # 搜索 50 次迭代
    for _ in range(50):
        epsilon = epsilon_factor * arc_length
        approx_polygon = cv2.approxPolyDP(main_contour, epsilon, True)
        current_count = len(approx_polygon)

        # 检查是否“足够接近”（例如，在目标
        if abs(current_count - target_count) < target_count * 0.1:  # 10% 容差
            best_polygon = approx_polygon
            best_epsilon_factor = epsilon_factor
            break # 找到了一个好结果

        # 调整 epsilon_factor
        if current_count > target_count:
            # 顶点太多，需要更多简化 (增加 epsilon)
            epsilon_factor *= 1.1
        else:
            # 顶点太少，需要 更少简化 (减少 epsilon)
            epsilon_factor *= 0.9
        
        best_polygon = approx_polygon
        best_epsilon_factor = epsilon_factor

    vertices = best_polygon.reshape(-1, 2).tolist()
    print(f"  > Got {len(vertices)} vertices (Epsilon: {best_epsilon_factor:.6f})")
    return vertices, best_epsilon_factor

# -----------------------------------------------------------------
# 主执行流程
# -----------------------------------------------------------------

# ---顶点密度控制 ---
# 望每个多边形 大致 拥有的顶点数。
BASE_TARGET_COUNT = 300 

# --- GUI 文件选择 ---
path_a = select_file(title="Select FIRST image (Polygon A)")
path_b = select_file(title="Select SECOND image (Polygon B)")

output_path_a = "../assets/poly_a.json"
output_path_b = "../assets/poly_b.json"

# --- 第一次提取 ---
# 尝试获取接近 BASE_TARGET_COUNT 的顶点
poly_a_vertices, epsilon_a = find_polygon_near_target(path_a, BASE_TARGET_COUNT)
poly_b_vertices, epsilon_b = find_polygon_near_target(path_b, BASE_TARGET_COUNT)

m = len(poly_a_vertices)
n = len(poly_b_vertices)

# --- 强制 m >= n ---
while m < n:
    print(f"(!) Constraint m >= n FAILED (A={m}, B={n}).")
    print("    Simplifying Polygon B further...")
    
    # B 的顶点太多了。我们需要 增加 它的 epsilon 来减少顶点
    epsilon_b *= 1.05  # 增加 5% 的简化
    
    # 使用 简单 的 extract_polygon 重新提取 B
    poly_b_vertices = extract_polygon(path_b, epsilon_b)
    n = len(poly_b_vertices)
    print(f"  > B is now {n} vertices.")

print("-" * 20)
print("Constraint m >= n PASSED.")

# 保存为JSON
with open(output_path_a, "w") as f:
    json.dump(poly_a_vertices, f)

with open(output_path_b, "w") as f:
    json.dump(poly_b_vertices, f)

print(f"提取了 {len(poly_a_vertices)} 个顶点到 {output_path_a}")
print(f"提取了 {len(poly_b_vertices)} 个顶点到 {output_path_b}")

# 可视化 
print("\n按任意键关闭预览窗口...")
img_a_display = cv2.imread(path_a)
img_b_display = cv2.imread(path_b)

if poly_a_vertices:
    contour_a_np = np.array(poly_a_vertices, dtype=np.int32).reshape((-1, 1, 2))
    cv2.drawContours(img_a_display, [contour_a_np], -1, (0, 255, 0), 2)
    cv2.imshow("Image A - Contour", img_a_display)

if poly_b_vertices:
    contour_b_np = np.array(poly_b_vertices, dtype=np.int32).reshape((-1, 1, 2))
    cv2.drawContours(img_b_display, [contour_b_np], -1, (0, 0, 255), 2)
    cv2.imshow("Image B - Contour", img_b_display)

cv2.waitKey(0)
cv2.destroyAllWindows()
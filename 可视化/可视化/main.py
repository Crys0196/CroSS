from getClusteringMap import get_clustering_map
from getmap import visualize_coordinates_on_map
from resizeImage import resize_image

# 从txt文件中读取经纬度坐标点
'''
with open('D:\\OneDrive\\桌面\\文件\\北斗杯\\可视化\\raw_data\\Branch_line.txt', 'r') as file:
    lines = file.readlines()

latitude = []
longitude = []

# 提取经纬度坐标点
for line in lines:
    if line.startswith('%'):  # 跳过以 "%" 开头的注释行
        continue
    cols = line.strip().split()  # 以空格分割每行的列
    lat = float(cols[2])  # 第3列是纬度
    lon = float(cols[3])  # 第4列是经度
    latitude.append(lat)  # append实现在列表的末尾增添一个元素
    longitude.append(lon)

# 打印经纬度坐标点
# for i in range(len(latitude)):
    # print(f"Point {i + 1}: Latitude = {latitude[i]}, Longitude = {longitude[i]}")
'''

# 绘制地图
'''
plt.figure(figsize=(10, 6))
plt.scatter(longitude, latitude, color='red', marker='o')
plt.xlabel('Longitude')
plt.ylabel('Latitude')
plt.title('Trajectory Visualization')
plt.grid(True)
plt.show()
'''

# 强制缩放图像
max_size = 260
image_paths = [r'./image/火星营地.jpg',
               r'./image/火星营地.jpg',
               r'./image/火星营地.jpg']
output_image_paths = [r'./image/after_zoom/image4_ebl.jpg',
                      r'./image/after_zoom/image6_ebl.jpg',
                      r'./image/after_zoom/image5_sjtu.jpg']
resize_image(image_paths, max_size, output_image_paths)

# 定义聚类路线
'''
clustering_paths = [r'./raw_data2/T1.txt',
                    r'./raw_data2/T2.txt',
                    r'./raw_data2/T3.txt']
'''
clustering_paths = [r'./raw_data2/T12.txt']

# 原式数据
'''
ori_paths = [r"D:\OneDrive\桌面\文件\北斗杯\可视化\GNSSdandian_huoxingyingdi\小组数据\第二次\1.txt",
             r"D:\OneDrive\桌面\文件\北斗杯\可视化\GNSSdandian_huoxingyingdi\小组数据\第二次\2.txt",
             r"D:\OneDrive\桌面\文件\北斗杯\可视化\GNSSdandian_huoxingyingdi\小组数据\第二次\3.txt"]
'''

# 在地图中显示路线
# visualize_coordinates_on_map(ori_paths, 'red')

# 显示聚类后的不同路线
get_clustering_map(clustering_paths, output_image_paths)

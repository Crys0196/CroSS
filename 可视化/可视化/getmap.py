import folium


def visualize_coordinates_on_map(ori_paths, p_color):
    # 颜色
    colos = ['red', 'blue', 'green']
    i = 0
    # 创建地图对象
    get_map = folium.Map(location=[31.025227099,  121.440655199], zoom_start=100)

    for path in ori_paths:
        # print(path)
        with open(path, 'r') as file:
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
        # 添加路径线条
        path = list(zip(latitude, longitude))
        # print(colos[i % 3])
        folium.PolyLine(path, color=colos[i % 3], weight=2.5, opacity=1).add_to(get_map)
        i = i + 1

    # 添加经纬度坐标点
    #for lat, lon in zip(latitude, longitude):
        #folium.Marker(location=[lat, lon], icon=folium.Icon(color=p_color)).add_to(get_map)

    # 保存地图为HTML文件
    get_map.save('map.html')


# 示例用法
'''
latitude = [30.2672, 30.2669, 30.2675, 30.2678]
longitude = [-97.7431, -97.7433, -97.7441, -97.7445]

visualize_coordinates_on_map(latitude, longitude)
'''

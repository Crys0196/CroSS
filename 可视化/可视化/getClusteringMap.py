import folium

from showContent import show_content


def get_clustering_map(clustering_paths, image_paths):
    # 创建地图对象
    get_map = folium.Map(location=[31.025846646, 121.438451689], zoom_start=500)

    # 定义颜色
    path_color = ['red', 'blue', 'green', 'black', 'pink']
    index_color = 0

    for clustering_path in clustering_paths:
        ifmap = 1
        # 读取cfa路径下文件内每行的经纬度坐标
        with open(clustering_path, 'r') as file:
            lines = file.readlines()

        # 将聚类的经纬数据按组分类
        data = {}
        for line in lines:
            # if line.startswith('%'):  # 跳过以 "%" 开头的注释行
            # continue
            cols = line.strip().split(',')  # 以逗号分割每行的列
            lon = float(cols[0])  # 第1列是经度
            lat = float(cols[1])  # 第2列是维度
            group = int(cols[2])  # 第3列是组号

            if group not in data:
                # print(f"{group}")
                data[group] = []
                data[group].append([])
                data[group].append([])

            data[group][0].append(lat)
            data[group][1].append(lon)

        # 打印每组的经纬度
        '''
        for group, coord in data.items():
            print(f"Group {group}: ")
            for i in range(len(data[group][1])):
                # 此代码i从0开始
                print(f"latitude is {data[group][0][i]}, and longitude is {data[group][1][i]}")
            print("\n")
        '''

        # 添加路径条
        for group, coord in data.items():

            if len(data[group][0]) > 15:
                path = list(zip(data[group][0], data[group][1]))

                # folium.PolyLine(path, popup="aa", color=path_color[index_color % 5], weight=5, opacity=0.8).add_to(
                # get_map)

                # 创建路径对象
                get_polyline = folium.PolyLine(path, color=path_color[index_color % 5], weight=5, opacity=0.8)
                get_polyline.add_to(get_map)

                '''
                # 添加所有经纬度坐标点
                for lat, lon in zip(data[group][0], data[group][1]):
                    folium.Marker(location=[lat, lon], icon=folium.Icon(color=path_color[index_color])).add_to(map)
                '''

                # 添加各组起点与终点icon maker
                folium.Marker(location=[data[group][0][0], data[group][1][0]],
                              icon=folium.Icon(color=path_color[index_color % 5])).add_to(get_map)
                folium.Marker(
                    location=[data[group][0][len(data[group][0]) - 1], data[group][1][len(data[group][1]) - 1]],
                    icon=folium.Icon(color=path_color[index_color % 5])).add_to(get_map)

                index_color = index_color + 1

                ifmap = ifmap + 1

        # 添加地图的可点击内容
        show_content(get_map, data, image_paths)

    # 保存地图为HTML文件
    get_map.save('clu_map.html')
    print('clu_map.html has been produced.')

    # 解决地图空白问题
    search_text = "cdn.jsdelivr.net"
    replace_text = "gcore.jsdelivr.net"
    # replace_text = "fastly.jsdelivr.net"
    # 如果还是不行就把上面的注释取消掉
    # 使用 open() 函数以只读模式打开我们的文本文件
    with open(r'clu_map.html', 'r', encoding='UTF-8') as file:
        # 使用 read() 函数读取文件内容并将它们存储在一个新变量中
        data = file.read()
        # 使用 replace() 函数搜索和替换文本
        data = data.replace(search_text, replace_text)
    # 以只写模式打开我们的文本文件以写入替换的内容
    with open(r'clu_map.html', 'w', encoding='UTF-8') as file:
        # 在我们的文本文件中写入替换的数据
        file.write(data)


'''
    # 添加路径线条
    path = list(zip(latitude, longitude))
    folium.PolyLine(path, color='blue', weight=2.5, opacity=1).add_to(map)

    # 添加经纬度坐标点
    for lat, lon in zip(latitude, longitude):
        folium.Marker(location=[lat, lon], icon=folium.Icon(color=p_color)).add_to(map)

    # 保存地图为HTML文件
    map.save('map.html')
'''

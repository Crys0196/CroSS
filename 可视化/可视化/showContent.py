import folium
import base64

from getImageSize import get_image_size


def show_content(get_map, data, image_paths0):
    # 定义image_paths
    image_paths = image_paths0
    num_image = len(image_paths)

    # 得到图像大小
    image_sizes = get_image_size(image_paths)
    index_color = 0
    for group, coord in data.items():

        if len(data[group][0]) > 50:
            # print('这是第', index_color + 1, '个照片')
            # 定义image_maker
            encoded = base64.b64encode(open(image_paths[index_color % num_image], 'rb').read()).decode()
            html = '<img src="data:image/jpeg;base64,{}">'.format
            iframe = folium.IFrame(html(encoded), width=image_sizes[index_color % num_image][0],
                                   height=image_sizes[index_color % num_image][1])

            # popup = folium.Popup(iframe, max_width=2650)
            popup = folium.Popup(iframe, max_width=2650)
            marker = folium.Marker(location=[data[group][0][int(len(data[group][0]) / 2)],
                                             data[group][1][int(len(data[group][0]) / 2)]], popup=popup).add_to(get_map)
            get_map.add_child(marker)

            index_color = index_color + 1

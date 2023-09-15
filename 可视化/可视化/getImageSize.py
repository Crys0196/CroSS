from PIL import Image

from resizeImage import resize_image


# 输入图像路径, 输出图像大小
def get_image_size(image_paths):

    len_image_paths = len(image_paths)

    # [width][length]
    rows = len_image_paths
    cols = 2
    image_size = [[0] * cols for _ in range(rows)]

    index = 0

    # 打开图像文件
    for image_path in image_paths:
        image = Image.open(image_path)

        # 获取图像的宽度和高度
        image_size[index][0], image_size[index][1] = image.size

        index = index + 1

    return image_size

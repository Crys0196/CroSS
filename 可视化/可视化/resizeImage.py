from PIL import Image


# 输入输入图片路径, 最大尺寸和输出图片路径, 函数将resize后的图片保存在output_image_paths中
def resize_image(input_image_paths, max_size, output_image_paths):
    i = 0
    # 打开输入图像
    for input_image_path in input_image_paths:
        image = Image.open(input_image_path)

        # 计算宽度和高度的缩放比例
        width, height = image.size
        if width > height:
            new_width = max_size
            new_height = int(height * max_size / width)
        else:
            new_width = int(width * max_size / height)
            new_height = max_size

        # 缩放图像
        resized_image = image.resize((new_width, new_height))

        # 保存缩放后的图像
        resized_image.save(output_image_paths[i])

        i = i + 1

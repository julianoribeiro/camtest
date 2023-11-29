import os
from PIL import Image
import numpy as np

def rgb565_to_rgb888(byte1, byte2):
    """Convert a color from RGB565 to RGB888."""
    rgb565 = (byte1 << 8) | byte2
    red = ((rgb565 >> 11) & 0x1F) * 255 / 31
    green = ((rgb565 >> 5) & 0x3F) * 255 / 63
    blue = (rgb565 & 0x1F) * 255 / 31
    return int(red), int(green), int(blue)

def convert_rgb565_to_jpeg(input_file, output_file, width, height):
    """Convert an RGB565 binary file to a JPEG image."""
    with open(input_file, 'rb') as file:
        binary_data = file.read(width * height * 2)

    image_data = np.zeros((height, width, 3), dtype=np.uint8)
    for i in range(0, len(binary_data), 2):
        row = (i // 2) // width
        col = (i // 2) % width
        r, g, b = rgb565_to_rgb888(binary_data[i], binary_data[i + 1])
        image_data[row, col] = [r, g, b]

    image = Image.fromarray(image_data, 'RGB')
    image.save(output_file)

def convert_all_bin_in_folder(folder_path, width, height):
    """Convert all .bin files in a folder to .jpg format."""
    for filename in os.listdir(folder_path):
        if filename.endswith(".bin"):
            input_file = os.path.join(folder_path, filename)
            output_file = os.path.join(folder_path, filename[:-4] + '.jpg')
            try:
                convert_rgb565_to_jpeg(input_file, output_file, width, height)
                print(f"Converted {input_file} to {output_file}")
            except Exception as e:
                print(f"Failed to convert {input_file}. Error: {e}")

# Exemplo de uso
folder_path = "."  # Substitua pelo caminho da pasta
width, height = 240, 240  # Substitua pelas dimensões corretas da imagem

convert_all_bin_in_folder(folder_path, width, height)

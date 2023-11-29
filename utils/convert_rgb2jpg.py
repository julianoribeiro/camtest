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

# Exemplo de uso do script
input_file_path = 'foto.bin'
output_file_path = 'foto.jpeg'
width, height = 240, 240  # Substitua pelas dimensões corretas da imagem

convert_rgb565_to_jpeg(input_file_path, output_file_path, width, height)

import serial
from read_from_port import *
from PIL import Image

def main():
    width = 640
    height = 480
    pixels = readFromPort("COM3", 9600, width, height)
    img = Image.new('RGB', (width, 4))
    img.save('created_img.png')

    for y in range(height):
        for x in range(width):
            img.putpixel((x, y), pixels[width*y+x])
    img.save('created_img.png')
    img.show()

if __name__ == "__main__":
    main()
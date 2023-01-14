import serial
from PIL import Image


def complByte(byte): #функция для дополнения байтов до 8-значной формы
    byte = byte[2:]
    if len(byte) < 8:
        byte = (8 - len(byte)) * "0" + byte
    return byte

def connectBytes(array): #соединение байтов
    array1 = array[::2]
    array2 = array[1::2]
    array = []
    for i in range(len(array1)):
        array.append(array1[i] + array2[i])
    return array

def transformToRGB888(pixelByte): # функция форматирования значений пикселей в RGB888
    red = round(int(pixelByte[:5], 2) / 31 * 255)
    green = round(int(pixelByte[5:11], 2) / 63 * 255)
    blue = round(int(pixelByte[11:], 2) / 31 * 255)
    pixelByte = (red, green, blue)
    return pixelByte

def readFromPort(portName, baudrate, width, height): #функция чтения и форматирования данных с порта
    ser = serial.Serial(portName, baudrate)

    ser.write(255)
    
    while True: 
        if ser.read(1) == b'\xbb':
            m = ser.read(width * height * 2)
            break
    
    ser.close()
    pixels = list(m) #преобразование данных в список десятичных чисел
    pixels = list(map(bin, pixels)) #каждый байт в двоичное сичло
    pixels = list(map(complByte, pixels)) #дополнение байтов до 8-значной формы
    pixels = connectBytes(pixels) #соединение байтов
    pixels = list(map(transformToRGB888, pixels))#форматирование  значений пикселей в RGB888

    return pixels

def createImage(width, height, portName, baudrate):
    img = Image.new('RGB', (width, height))
    img.save('created_img.png')
    try:
        pixels = readFromPort(portName, baudrate, width, height)

        for y in range(height):
            for x in range(width): 
                img.putpixel((x, y), pixels[width*y+x])
        img.save('created_img.png')
    except:
        print("no COM_device")

    
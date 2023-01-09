import PySimpleGUI as sg
from ImageFromPort import *
import serial
from PIL import Image

img = Image.new('RGB', (640, 480))
img.save('created_img.png')

sg.theme('DarkAmber')


layout = [[sg.Image('created_img.png'), sg.Image(key = '-OUTPUT-')],
          [sg.Button('Create'), sg.Exit()],
          [sg.InputText(key='-COMMAND-'), sg.Button('Send')]]

window = sg.Window('Control panel', layout)
#eventloop
while True:
    event, values = window.read()
    print(event, values)
    if event == sg.WIN_CLOSED or event == 'Exit':
        break
    if event == 'Create':
        ser = serial.Serial('COM3', 9600)
        ser.write(255)
        createImage(640, 480, 'COM3', 9600)
        window['-OUTPUT-'].update()
    if event == 'Send':
        try:
            ser = serial.Serial('COM3', 9600)
            ser.write(int(values['-COMMAND-']))
            print('Command was sended')
        except:
            print('Something went wrong')
window.close()
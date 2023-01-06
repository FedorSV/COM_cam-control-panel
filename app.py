import PySimpleGUI as sg
from ImageFromPort import *
createImage(160, 120)
sg.theme('DarkAmber')

layout = [[sg.Image('created_img.png'), sg.Image(key = '-OUTPUT-')],
          [sg.Button('Create'), sg.Exit()]]

window = sg.Window('Control panel', layout)
#eventloop
while True:
    event, values = window.read()
    print(event, values)
    if event == sg.WIN_CLOSED or event == 'Exit':
        break
    if event == 'Create':
        createImage(160, 120)
        window['-OUTPUT-'].update()
window.close()
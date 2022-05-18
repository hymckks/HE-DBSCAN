import tkinter as tk
from tkinter import *
from PIL import Image, ImageTk
import os
import subprocess

window = tk.Tk()
window.title('privacy-preserving clustering')
window.geometry('870x600')

canvas = tk.Canvas(window, height=400, width=800)
imagefile = ImageTk.PhotoImage(file='hqucs.jpeg')
image = canvas.create_image(300, 0, anchor='nw', image=imagefile)
canvas.pack(side='top')
# ------------------------------------------------------------------------------------------------
var = tk.StringVar()
label1 = tk.Label(window, textvariable=var, bg='white', font=('Arial', 15), width=80, height=2)
label1.place(x=-25, y=220)
var.set('Click the left “Button” to start clustering')
# ------------------------------------------------------------------------------------------------
Text1 = tk.Text(window, width=50)
Text1.place(x=230, y=300)  # 230 300
# ---------------------------------------------------------------------------------------------

def start():
    os.system("cd /home/heyiming/Desktop/PPDBSCAN && ./ppdbscan")
    f = open('/home/heyiming/Desktop/PPDBSCAN/result.txt', encoding='gbk')
    for line in f:
        Text1.insert(END,line.strip())
        Text1.insert(END, '\n')

def end():
    Text1.delete('1.0', 'end')

def ready():
    os.system("cd /home/heyiming/Desktop/PPDBSCAN && ./makeppdbscan.sh")
    Text1.insert(END, 'success!!\n')
    Text1.insert(END, "please wait......\n")
# -------------------------------------------------------------------------------------------------------------
button1 = tk.Button(window, text='Start', bg='dodgerblue', font=('Arial', 8), width=30, height=4, command=start)
button1.place(x=15, y=300, anchor='nw')
button2 = tk.Button(window, text='Clear', bg='dodgerblue', font=('Arial', 8), width=30, height=4,command=end)
button2.place(x=650, y=400, anchor='nw')

button3 = tk.Button(window, text='Ready', bg='dodgerblue', font=('Arial', 8), width=30, height=4,command=ready)
button3.place(x=15, y=500, anchor='nw')
window.mainloop()
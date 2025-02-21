import tkinter as tk  
from tkinter import messagebox, simpledialog  
import json  
import os  
  
# 假设这是您的JSON文件的路径  
json_file_path = 'coordinates.json'  
  
# 读取JSON文件  
def read_json_file(file_path):  
    with open(file_path, 'r', encoding='utf-8') as file:  
        data = json.load(file)  
    return data  
  
# 保存JSON数据到文件  
def write_json_file(file_path, data):  
    with open(file_path, 'w', encoding='utf-8') as file:  
        json.dump(data, file, ensure_ascii=False, indent=4)  
  
# 更新列表框显示  
def update_listbox(listbox, data):  
    listbox.delete(0, tk.END)  # 清空列表框  
    for index, item in enumerate(data):  
        listbox.insert(tk.END, f"Index {index + 1}: x={item['x']}, y={item['y']}")  
  
# 初始化数据和Tkinter界面  
def init_app():  
    global data, listbox, entry_x, entry_y, root  
  
    # 读取JSON数据  
    data = read_json_file(json_file_path)  
  
    # 创建Tkinter窗口  
    root = tk.Tk()  
    root.title("JSON Coordinate Editor")  
  
      # 创建一个Frame来放置Listbox和Scrollbar  

    frame_listbox = tk.Frame(root)  

    frame_listbox.pack()  

    # 创建一个列表框来显示坐标点  
    listbox = tk.Listbox(frame_listbox, width=50, height=10)  
    listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)  # 让Listbox扩展以填充Frame  
    # 创建滚动条并关联到Listbox  
    scrollbar = tk.Scrollbar(frame_listbox, orient=tk.VERTICAL, command=listbox.yview)  
    scrollbar.pack(side=tk.RIGHT, fill=tk.Y)  
    listbox.config(yscrollcommand=scrollbar.set)  # 设置Listbox的滚动命令到Scrollbar  
    
    # 更新列表框显示  
    update_listbox(listbox, data)  
  
    # 创建输入框和按钮来添加新的坐标点  
    frame = tk.Frame(root)  
    frame.pack()  
  
    label_x = tk.Label(frame, text="x:")  
    label_x.pack(side=tk.LEFT)  
    entry_x = tk.Entry(frame)  
    entry_x.pack(side=tk.LEFT)  
  
    label_y = tk.Label(frame, text="y:")  
    label_y.pack(side=tk.LEFT)  
    entry_y = tk.Entry(frame)  
    entry_y.pack(side=tk.LEFT)  
  
    local_button = tk.Button(frame, text="Local")  
    local_button.pack(side=tk.RIGHT)  
  
    # 创建按钮来修改选中的坐标点  
    modify_button = tk.Button(root, text="Modify", command=modify_coordinate)  
    modify_button.pack(side=tk.LEFT)  
  
    # 创建按钮来删除选中的坐标点  
    delete_button = tk.Button(root, text="Delete", command=delete_coordinate)  
    delete_button.pack(side=tk.LEFT)  

    add_button = tk.Button(root, text="ADD", command=add_coordinate)  
    add_button.pack(side=tk.LEFT)  
  
    # 创建按钮来保存修改后的数据到文件  
    save_button = tk.Button(root, text="Save", command=save_data)  
    save_button.pack(side=tk.RIGHT)  
  
    # 运行Tkinter事件循环  
    root.mainloop()  
  
# 添加新的坐标点  
def add_coordinate():  
    try:  
        x = float(entry_x.get())  
        y = float(entry_y.get())  
        new_coord = {"x": x, "y": y}  
        data.append(new_coord)  
        update_listbox(listbox, data)  
        entry_x.delete(0, tk.END)  
        entry_y.delete(0, tk.END)  
        listbox.yview(tk.END)  # 滚动到底部  
    except ValueError:  
        messagebox.showerror("Input Error", "Please enter valid numbers for x and y.")  
  
# 修改选中的坐标点  
def modify_coordinate():  
    try:  
        index = listbox.curselection()[0]  
        x = float(entry_x.get())  
        y = float(entry_y.get())  
        data[index]['x'] = x  
        data[index]['y'] = y  
        update_listbox(listbox, data)  
        entry_x.delete(0, tk.END)  
        entry_y.delete(0, tk.END)  
    except (IndexError, ValueError):  
        messagebox.showerror("Input Error", "Please select a valid index and enter valid numbers for x and y.")  
  
# 删除选中的坐标点  
def delete_coordinate():  
    try:  
        index = listbox.curselection()[0]  
        del data[index]  
        update_listbox(listbox, data)  
    except IndexError:  
        messagebox.showerror("Selection Error", "Please select a valid index to delete.")  
  
# 保存修改后的数据到文件  
def save_data():  
    write_json_file(json_file_path, data)  
    messagebox.showinfo("Success", "Data saved successfully.")  
  
# 运行应用程序  
if __name__ == "__main__":  
    # 确保JSON文件存在且格式正确（可以先手动创建一个正确的文件）  
    if not os.path.exists(json_file_path):  
        with open(json_file_path, 'w', encoding='utf-8') as file:  
            json.dump([{"x": 11.0, "y": 7.0}], file, ensure_ascii=False, indent=4)  
    init_app()
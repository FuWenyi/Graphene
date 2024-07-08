# -*- coding: utf-8 -*-

import csv

# 打开原始文件并读取数据
with open('iostat_spmv_2x2_1.log', 'r') as file:
    lines = file.readlines()

# 定义要提取的列名
columns = ['%user', 'nice', '%system', '%iowait', '%steal', '%idle']

# 初始化一个空的数据列表
data = []

# 标记是否找到了"avg-cpu"
found_avg_cpu = False

# 遍历文件的每一行
for line in lines:
    # 如果当前行包含"avg-cpu"，将标记设置为True
    if 'avg-cpu' in line:
        found_avg_cpu = True
    # 如果找到了"avg-cpu"，并且当前行不包含"avg-cpu"，则获取当前行的数据
    elif found_avg_cpu:
        # 以空格分割当前行，并提取出我们感兴趣的数据
        values = line.split()[0:6]
        # 将提取到的数据转换为浮点数
        values = [float(value) for value in values]
        # 将数据添加到数据列表中
        data.append(values)
        # 重置标记为False，以便下次找到"avg-cpu"
        found_avg_cpu = False

# 将提取的数据写入 CSV 文件
with open('cpu_output.csv', 'w') as csvfile:
    writer = csv.writer(csvfile)
    # 写入列名
    writer.writerow(columns)
    # 写入数据
    writer.writerows(data)

print("数据已成功提取并写入 output.csv 文件。")

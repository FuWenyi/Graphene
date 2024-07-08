# -*- coding: utf-8 -*-
import csv
import re

# 打开输入的文本文件
with open('input.txt', 'r') as file:
    data = file.readlines()

# @level-1-font-leveltime-converttm-iotm-waitiotm-waitcomptm-iosize: 469 0.000522852 2.86102e-06 0(0.000468969,5.10216e-05)  0.000509977 0 2560 9.05991e-06
# 打开 CSV 文件进行写入
with open('output.csv', 'w') as csvfile:
    writer = csv.writer(csvfile, delimiter=',')
    
    # 写入标题行
    writer.writerow(['level', 'frontier num', 'level tm', 'convert tm', 'io tm', 'io submit tm', 'io poll tm', 'wait io tm', 'wait comp tm', 'iosize', 'comp tm end'])
    
    # 写入数据行
    for line in data:
        parts = line.split()
        level_match = re.search(r'level-(\d+)', parts[0])  # 使用正则表达式提取 level-x 中的数字
        level = level_match.group(1) if level_match else None
        fron = parts[1]
        time = parts[2]
        converttm = parts[3]
        parenthesis_match = re.search(r'(.+)\((.+),(.+)\)', parts[4])
        iotm = parenthesis_match.group(1)
        io_sub_tm = parenthesis_match.group(2) if parenthesis_match else None
        io_poll_tm = parenthesis_match.group(3) if parenthesis_match else None
        waitiotm = parts[5]  # 去除括号
        waitcomptm = parts[6]
        iosize = parts[7]
        compend = parts[8]
        
        writer.writerow([level, fron, time, converttm, iotm, io_sub_tm, io_poll_tm, waitiotm, waitcomptm, iosize, compend])

print("CSV 文件写入完成")
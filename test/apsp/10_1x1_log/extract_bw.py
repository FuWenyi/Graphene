# -*- coding: utf-8 -*-

import csv

# 打开日志文件和CSV文件
with open('iostat_apsp12.log', 'r') as log_file, open('dw_output.csv', 'w', newline='') as csvfile:
    # 设置CSV写入器和列名
    writer = csv.writer(csvfile)
    writer.writerow(['rMB/s', 'device_util'])

    # 逐行读取日志文件并解析数据
    for line in log_file:
        # 使用空格将每行数据分割成列表
        if "nvme0n1" in line:
          data = line.split()
          device = data[0]
          # 检查行中是否包含"nvme0n1"
          if "nvme0n1" == device:
  
              # 检查列表长度，确保数据完整
              if len(data) >= 13:
                  # 提取第5列（索引为4）和第13列（索引为12）的数据
                  rKB_s = float(data[5])
                  rMB_s = rKB_s / 1000
                  util = float(data[13])
  
                  # 将提取的数据写入CSV文件
                  writer.writerow([rMB_s, util])

# -*- coding: utf-8 -*-

import csv
import re

'''
Total time: 10.5618 second(s)
Total io time: 9.21052 second(s)
Total comp time: 3.64228 second(s)
'''

# 打开日志文件和CSV文件
with open('random_20_3x4.log', 'r') as log_file, open('random_20_3x4_time_output.csv', 'w') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(['total', 'io', 'comp'])

    # 逐行读取日志文件并解析数据
    for line in log_file:
        if "Total time:" in line:
            total = re.search(r'Total time: (\d+\.\d+) second\(s\)', line).group(1)
        elif "Total io time:" in line:
            io = re.search(r'Total io time: (\d+\.\d+) second\(s\)', line).group(1)
        elif "Total comp time:" in line:
            comp = re.search(r'Total comp time: (\d+\.\d+) second\(s\)', line).group(1)
            # 写入CSV文件
            writer.writerow([total, io, comp])
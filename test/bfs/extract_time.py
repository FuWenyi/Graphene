# -*- coding: utf-8 -*-

import csv
import re

'''
Total time: 10.5618 second(s)
Total io time: 9.21052 second(s)
Total comp time: 3.64228 second(s)
'''

# ����־�ļ���CSV�ļ�
with open('random_20_3x4.log', 'r') as log_file, open('random_20_3x4_time_output.csv', 'w') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(['total', 'io', 'comp'])

    # ���ж�ȡ��־�ļ�����������
    for line in log_file:
        if "Total time:" in line:
            total = re.search(r'Total time: (\d+\.\d+) second\(s\)', line).group(1)
        elif "Total io time:" in line:
            io = re.search(r'Total io time: (\d+\.\d+) second\(s\)', line).group(1)
        elif "Total comp time:" in line:
            comp = re.search(r'Total comp time: (\d+\.\d+) second\(s\)', line).group(1)
            # д��CSV�ļ�
            writer.writerow([total, io, comp])
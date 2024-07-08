# -*- coding: utf-8 -*-
import random

def generate_random_numbers(n):
    random_numbers = []
    for _ in range(20):
        random_numbers.append(random.randint(0, n))
    return random_numbers

given_number = int(input("type in vertex count: "))
random_numbers = generate_random_numbers(given_number)
print("random 20: ", random_numbers)
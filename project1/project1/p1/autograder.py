#!/usr/bin/env python3

"""
Notes: 
- You should get 0 or very few differences. Current input files have few repetitive value. This will not happen in the grading benchmarks.
- Running the autograder should take less than 5 minutes. (it should be around 30 seconds)
- Number of entries in the input files might vary the grading benchmarks.

"""
import subprocess
import os
import sys
try:
    import pandas as pd
except ModuleNotFoundError:
    print('[ERROR] Missing dependency: pandas')
    print('[HINT] Install it with: python3 -m pip install pandas')
    sys.exit(1)
from pathlib import Path

print('Compiling your program')

# run make
make_process = subprocess.Popen("make clean && make", shell=True, stdout=subprocess.PIPE, stderr=sys.stdout.fileno())
make_process.wait()
    
print('Your program was successfully compiled')
print('Running your program now...')

# Ensure output directory exists so student code can write results.
Path('output').mkdir(exist_ok=True)

# run user program
exec_process = subprocess.Popen("./p1_exec 5 6",shell=True, stdout=subprocess.PIPE, stderr=sys.stdout.fileno())
exec_process.wait()

print('Checking your outputs')

input_path = Path('input/')
ground_truth_path = Path('grader_outputs/')
user_path = Path('output/')

input_files = [file for file in input_path.iterdir() if file.suffix == '.csv']
for input_csv in input_files:
    test_name = input_csv.stem
    if ground_truth_path.joinpath(f'{test_name}_stats.csv').exists():
        correct_outputs_stats = pd.read_csv(ground_truth_path.joinpath(f'{test_name}_stats.csv').as_posix()).round(3)
        outputs_stats = pd.read_csv(user_path.joinpath(f'{test_name}_stats.csv').as_posix()).round(3)

        print(f'Stats for {test_name} test match? {str(correct_outputs_stats.equals(outputs_stats))}')
    
    if ground_truth_path.joinpath(f'{test_name}_sorted.csv').exists(): 
        correct_outputs_sorted = pd.read_csv(ground_truth_path.joinpath(f'{test_name}_sorted.csv'))[['Rank', 'Student ID']]
        outputs_sorted = pd.read_csv(user_path.joinpath(f'{test_name}_sorted.csv'))[['Rank', 'Student ID']]
        differences = correct_outputs_sorted.count().sum() - (correct_outputs_sorted == outputs_sorted).astype(int).sum().sum()
    
        print(f'Differences in rows for {test_name} test? {differences}\n');


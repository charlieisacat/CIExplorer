import os
from common import is_dir_empty
from config import base_dir 

def read_file_to_dict(file_path):
    result_dict = {}
    with open(file_path, "r") as file:
        for line in file:
            key, value = line.strip().split("=")
            try:
                # Try converting to float if possible, otherwise keep as string
                result_dict[key] = float(value)
            except ValueError:
                result_dict[key] = value
    return result_dict

def sort_dict_by_key(dict_to_sort):
    sorted_dict = dict(
        sorted(
            dict_to_sort.items(), key=lambda item: tuple(map(int, item[0].split("_")))
        )
    )
    return sorted_dict

benchmarks = [
    "aes/aes",
    "bfs/bulk",
     "bfs/queue",
   "fft/strided",
    "fft/transpose",
    "gemm/ncubed",
     "gemm/blocked",
    "kmp/kmp",
    "md/grid",
    "md/knn",
    "nw/nw",
    "sort/merge",
    "sort/radix",
     "spmv/crs",
     "spmv/ellpack",
  "stencil/stencil2d",
  "stencil/stencil3d",
   "viterbi/viterbi",
]

def getCustomExeInsns(data):
    n = 0
    for key in data.keys():
        value = data[key]
        if key.isnumeric():
            opcode = int(key)
            if opcode >= 1025:
                n += int(value)
                print(key, value)
    return n

speedups = []
areas = [] 
powers = [] 

for benchmark in benchmarks:
    print("\n", benchmark, "\n")
    bench, alg = benchmark.split("/")

    input_dir = os.path.join(base_dir, bench, alg)
    ori_stat_path = os.path.join(input_dir, "ori_stat.txt")
    ori_data = read_file_to_dict(ori_stat_path)
    
    benchmark_stat = {}
    novia_data = None
    flix_data = None
    s10_5_data = None
    genetic_data = None
    
    max_speedup = 0

    speedup_per_bench = {}

    for setting in os.listdir(input_dir):
        setting_path = os.path.join(input_dir, setting)

        if not os.path.isdir(setting_path):
            continue

        if is_dir_empty(setting_path):
            continue

        stat_path = os.path.join(setting_path, "stat.txt")
        if not os.path.exists(stat_path):
            continue
        data = read_file_to_dict(stat_path)
        
        speedup = ori_data["totalCycle"] / data["totalCycle"]
        
        if speedup > max_speedup:
            max_speedup = speedup


    speedups.append(max_speedup)
    
avg_speedup = sum(speedups) / len(speedups)
print("speedup", avg_speedup)

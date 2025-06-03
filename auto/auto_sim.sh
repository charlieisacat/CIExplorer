#!/bin/bash

exe="/root/nio/simulator/sim/build/my_program"
base_dir="/root/nio/auto/results"
benchmarks=(
    "aes/aes"
   "bfs/bulk" 
   "bfs/queue"
    "fft/strided"
    "fft/transpose"
    "gemm/ncubed"
     "gemm/blocked"
    "kmp/kmp"
    "md/grid"
    "md/knn"
    "nw/nw"
    "sort/merge"
    "sort/radix"
   "spmv/crs"
   "spmv/ellpack"
   "stencil/stencil2d"
   "stencil/stencil3d"
   "viterbi/viterbi"
)

gpp_type=ino
config_path="/root/nio/simulator/sim/${gpp_type}.yml"

is_dir_empty() {
    [ -z "$(ls -A "$1")" ]
}

for benchmark in "${benchmarks[@]}"; do
    IFS="/" read -r bench alg <<< "$benchmark"

    input_dir="$base_dir/$bench/$alg"

    run_ori=1
    for setting in $(ls "$input_dir"); do
        setting_path="$input_dir/$setting"

        if [ ! -d "$setting_path" ]; then
            continue
        fi

        if is_dir_empty "$setting_path"; then
            echo "empty $setting_path"
            continue
        fi        

        trace_path="$setting_path/example.txt"
        yaml_file="$setting_path/$setting.yaml"

        if [ $run_ori -eq 1 ]; then
            ir_path="$setting_path/${bench}_run_rn.bc"
            echo "$exe $ir_path $trace_path $config_path" "None" "None" "None" "None" 0 0
            eval "$exe $ir_path $trace_path $config_path" "None" "None" "None" "None" 0 0
            run_ori=0
            mv stat.txt $input_dir/ori_stat.txt
            mv mcpat.txt $input_dir/ori_mcpat.txt
        fi

        ir_path="$setting_path/${setting}.ll"

        # echo "$exe $ir_path $trace_path $config_path "None" "None" "None" "None" 0 1 $yaml_file"
        # eval "$exe $ir_path $trace_path $config_path "None" "None" "None" "None" 0 1 $yaml_file"
        echo "$exe $ir_path $trace_path $config_path "None" "None" "None" "None" 0 0 $yaml_file 1.0"
        eval "$exe $ir_path $trace_path $config_path "None" "None" "None" "None" 0 0 $yaml_file 1.0"

        mv stat.txt $setting_path/stat.txt
        mv mcpat.txt $setting_path/mcpat.txt
    done
done

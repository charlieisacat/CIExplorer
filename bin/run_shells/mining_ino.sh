#!/bin/bash
STEPS=200

config_file=$1
pack_n=$2

cp ${MINING_ROOT}/bin/executables/* ./ 

to_merge_file="to_merge"
mkdir to_merge

mkdir graphdata_step

python3 ${MINING_ROOT}/bin/preprocess/preprocess.py ../ddg_bb graphdata_bb vecnt_bb relabeldata_bb graphdata_step
mkdir split_bb_output


echo "./split_bb_ino ../bitcode/${BENCH}_run_rn.bc ../data/bblist.txt graphdata_step/graphdata_bb ../data/weights.txt $pack_n $config_file"
./split_bb_ino ../bitcode/${BENCH}_run_rn.bc ../data/bblist.txt graphdata_step/graphdata_bb ../data/weights.txt $pack_n $config_file

files=$(ls split_bb_output/*.patt)
for path in $files
do
    filename=${path##*/}
    basename="${filename%.*}"  # 去掉扩展名
    cp split_bb_output/${filename} ${to_merge_file}/${basename}.data
done 

mkdir merge_output
mkdir fu_ops_data
mkdir area_ops_data
# mkdir to_debug_pass
# ./merge to_merge/ ../data/weights.txt

mkdir cp_ops_data
# ./get_cp_ops ../bitcode/${BENCH}_run_rn.bc ./to_debug_pass 
./get_cp_ops ../bitcode/${BENCH}_run_rn.bc ./to_merge  $config_file 

cp -r to_merge to_debug_pass

echo "Visualization..."

rm -rf mining_output
rm -rf dfg_imgs

mkdir mining_output
mkdir dfg_imgs

./visualize ../bitcode/${BENCH}_run_rn.bc to_merge ../data/weights.txt
 ./vis.sh mining_output dfg_imgs

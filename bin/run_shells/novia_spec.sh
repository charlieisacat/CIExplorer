#!/bin/bash
FUSE_LIB=/staff/haoxiaoyu/uarch_exp/NOVIA/fusion/build/lib/

cp ${MINING_ROOT}/bin/executables/* ./ 

rm -rf graphdata
mkdir graphdata
python3 ${MINING_ROOT}/bin/preprocess/preprocess.py ../ddg_bb graphdata_bb vecnt_bb relabeldata_bb graphdata

name=$1
name_rn=../bitcode/${name}_run_rn.bc
name_novia=${name}_run_novia.bc

rm -rf output
mkdir output

rm -rf data
mkdir data

rm -rf fu_ops_data
mkdir fu_ops_data

rm -rf to_debug_pass
mkdir to_debug_pass
opt-14 -enable-new-pm=0 -load $FUSE_LIB/libfusionlib.so -mergeBBList -bbs \
	  "../data/bblist.txt" --dynInf "../data/weights.txt" --graph_dir imgs --visualLevel 7 \
	    --nfus nfu.txt < $name_rn > $name_novia

rm -rf cp_ops_data
mkdir cp_ops_data
rm -rf temp_output
mkdir temp_output

./parse_novia_output $name_rn to_debug_pass ./graphdata/graphdata_bb

rm -rf to_debug_pass
mv temp_output to_debug_pass

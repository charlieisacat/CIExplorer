#!/bin/bash

cp ${MINING_ROOT}/bin/executables/* ./ 
../../padding.sh

mkdir graphdata
python3 ${MINING_ROOT}/bin/preprocess/preprocess.py ./padding_output graphdata_bb vecnt_bb relabeldata_bb graphdata

rm -rf to_debug_pass
rm -rf fu_ops_data
rm -rf area_ops_data
rm -rf merge_output
rm -rf cp_ops_data

mkdir fu_ops_data
mkdir area_ops_data
mkdir to_debug_pass
mkdir merge_output
mkdir cp_ops_data

./extract_novia ./graphdata/graphdata_bb
# baseline won't limit io so set as large as possible
./merge baseline.data 1024 1024 0 ../data/weights.txt

./get_cp_ops to_debug_pass

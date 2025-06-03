#!/bin/bash
BENCH=$1

root=${MINING_ROOT}
prepare_lib="${root}/src/passes/build"
basepath=/staff/haoxiaoyu/uarch_exp/profile/SPEC
benchpath=$basepath/${BENCH}

name_rn=${BENCH}_run_rn.bc

echo $basepath
echo $benchpath

mkdir -p bitcode
mkdir -p data

mkdir ddg_bb

rm -rf patt_mining
mkdir patt_mining

cp $benchpath/data/test/input/bblist.txt data
cp $benchpath/data/test/input/weights.txt data
cp $benchpath/${BENCH}_rn.bc bitcode/$name_rn

opt-14 -enable-new-pm=0  -disable-output -load $prepare_lib/libdfg.so -bbDFG -outdir=ddg_bb -removeMemOp=0 -removeGepOp=0 -include data/bblist.txt bitcode/$name_rn

mv bitcode patt_mining
mv data patt_mining
mv ddg_bb patt_mining

mkdir patt_mining/temp

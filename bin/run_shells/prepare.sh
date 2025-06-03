#!/bin/bash

root=${MINING_ROOT}
prepare_lib="${root}/src/passes/build"

echo "############################################################################################"
echo "Configuration..."
echo "############################################################################################"
echo $MINING_ROOT
echo $EXECUTABLE
echo $EXECARGS
echo $EXCL_FILE

echo "############################################################################################"
echo "Start Running..."
echo "############################################################################################"

name_ext="$(basename $EXECUTABLE)"
name="${name_ext%.*}"
name_inl="bitcode/${name}_inl.bc"
name_reg="bitcode/${name}_reg.bc"
name_rn="bitcode/${name}_rn.bc"
name_ins="bitcode/${name}_ins.bc"
name_opt="bitcode/${name}_opt.bc"
name_unroll="bitcode/${name}_unroll.bc"
name_o="${name}_ins.o"
name_ins_bin="${name}_ins.bin"

excl_file=$EXCL_FILE
cp $EXECUTABLE $name_ext

INLINE=0

echo ${name_ext}
echo ${name}
echo ${name_inl}
echo ${name_reg}
echo ${name_rn}
echo ${name_ins}
echo ${name_o}
echo ${name_ins_bin}
echo ${name_unroll}

mkdir -p bitcode
mkdir -p data
mkdir -p output

mkdir ddg_bb
mkdir ddg_dep
mkdir ddg_func

rm -rf patt_mining
mkdir patt_mining

echo "############################################################################################"
echo "Analyzing $name: Preparation phase"
echo "############################################################################################"
opt-14 -enable-new-pm=0 -load $prepare_lib/libinliner.so -bottom-up-inline -targetFunc=run_benchmark $name_ext > $name_inl
opt-14 -enable-new-pm=0 -lowerswitch -lowerinvoke -mem2reg -simplifycfg < $name_inl > $name_opt

echo -n "Renaming: "
if [ ! -f $name_rn ]; then
  opt-14 -enable-new-pm=0  -load $prepare_lib/libprepare.so --renameBBs < $name_opt > "$name_rn"
  echo "done"
else
  echo "reused"
fi

echo -n "Instrumenting: "
if [ ! -f $name_ins ]; then
  opt-14 -enable-new-pm=0  -load $prepare_lib/libinstrument.so --instrumentBBs -excl ${excl_file} < $name_rn > $name_ins
  echo "done"
else
  echo "reused"
fi

echo -n "Linking Timers and Compiling: "
if [ ! -f $name_ins_bin ]; then
  clang++-14 $name_ins -L $prepare_lib -ltimer_x86 -o "$name_ins_bin"
  echo "done"
else
  echo "reused"
fi

echo -n "Profiling: ./$name_ins_bin:"
if [ ! -f data/histogram.txt ]; then
  EVAL=$(echo "./$name_ins_bin ${EXECARGS}")
  #EVAL=$(echo "./$name_ins_bin")
  eval $EVAL
  mv histogram.txt data/histogram.txt
  echo "done"
else
  echo "reused"
fi

echo "Processing Histogram"
python3 normalize.py data/histogram.txt data/weights.txt data/bblist.txt 0.90
if [ $? -ne 0 ]; then
  exit 1
fi

echo "Dump BB & Func DFG"
opt-14 -enable-new-pm=0  -disable-output -load $prepare_lib/libdfg.so -bbDFG -outdir=ddg_bb -removeMemOp=0 -removeGepOp=0  $name_rn
#opt -disable-output -enable-new-pm=0 -load $prepare_lib/libdfg.so -funcDFG -outdir=ddg_func -removeMemOp=0 -removeGepOp=0 -removePHIOp=0 $name_rn
opt-14 -enable-new-pm=0  -disable-output -load $prepare_lib/libdfg.so -inOutDep -outdir=ddg_dep -removeMemOp=0 -removeGepOp=0 -removePHIOp=0 $name_rn

mv bitcode patt_mining
mv data patt_mining
mv output patt_mining
mv ${name_ins_bin} patt_mining
mv ddg_bb patt_mining
mv ddg_func patt_mining
mv ddg_dep patt_mining

mkdir patt_mining/temp

rm -rf output.txt

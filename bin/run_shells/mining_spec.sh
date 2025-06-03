#!/bin/bash
STEPS=100

BENCH=$1

cp ${MINING_ROOT}/bin/executables/* ./ 

to_merge_file="to_merge"
mkdir to_merge

final_step=0
for step in $(seq $STEPS)
do
    echo -n "Step $step"
    echo ""
    cleandata_step_file="cleandata_step${step}"
    graphdata_step_file="graphdata_step${step}"
    gspan_input_step_file="gspan_input_step${step}"
    gspan_output_step_file="gspan_output_step${step}"
    ga_output_step_file="ga_output_step${step}"
    mia_step_file="mia_output_step${step}"
    rmops_input="rmops_input_step${step}"
    split2_attrs_input="split2_attrs_input_step${step}"
    split2_step_file="split2_output_step${step}"
    greedy_seed_output_step="greedy_seed_output${step}"
    split_bb_output_step="split_bb_output${step}"

    mkdir $graphdata_step_file

    if [ $step == 1 ]; then
        python3 ${MINING_ROOT}/bin/preprocess/preprocess.py ../ddg_bb graphdata_bb vecnt_bb relabeldata_bb $graphdata_step_file
    else
        step_prev=`expr $step - 1`
        mia_step_file_prev="mia_output_step${step_prev}"
        echo $mia_step_file_prev

        python3 ${MINING_ROOT}/bin/preprocess/preprocess.py ./${mia_step_file_prev} graphdata_bb vecnt_bb relabeldata_bb $graphdata_step_file
    fi
    echo "Clean data... "
    mkdir $cleandata_step_file
    ./rmdataops ../bitcode/${BENCH}_run_rn.bc $graphdata_step_file/graphdata_bb  $cleandata_step_file ../data/bblist.txt 
    # cp ${cleandata_step_file}/clean.data ${cleandata_step_file}/graphdata_bb

    mkdir $gspan_input_step_file
    python3 ${MINING_ROOT}/bin/preprocess/preprocess.py ./${cleandata_step_file} graphdata_bb vecnt_bb relabeldata_bb $gspan_input_step_file

    echo "Gspan..."
    support_th=2
    python3 ${MINING_ROOT}/src/gspan/gspan_mining/main.py -s $support_th -d True ./$gspan_input_step_file/relabeldata_bb

    mv gspan_output $gspan_output_step_file
    
    # ./extract_seed ../bitcode/${BENCH}_run_rn.bc ../data/bblist.txt $graphdata_step_file/graphdata_bb 1
    # mv seeds.data $gspan_output_step_file

    mkdir split_bb_output
    ./split_bb ../bitcode/${BENCH}_run_rn.bc ../data/bblist.txt $graphdata_step_file/graphdata_bb
    mv split_bb_output $split_bb_output_step

    gspan_output_n=`ls -l $gspan_output_step_file | grep -c "attrs.output"`
    # gspan_output_n2=`ls -l $gspan_output_step_file | grep -c "seeds.data"`
    # if [ $gspan_output_n == 0 ] && [ $gspan_output_n2 == 0 ]; then
    if [ $gspan_output_n == 0 ]; then
        files=$(ls $split_bb_output_step/*.patt)
        for path in $files
        do
            filename=${path##*/}
            basename="${filename%.*}"  # 去掉扩展名
            cp $split_bb_output_step/${filename} ${to_merge_file}/${step}_${basename}.data
        done 
        echo "gspan output is empty in step $step, skip following steps & do merge now"
        break
    fi
    final_step=$step

    echo "GA..."
    ${MINING_ROOT}/bin/run_shells/search.sh ../bitcode/${BENCH}_run_rn.bc $graphdata_step_file/graphdata_bb  $gspan_output_step_file $Nin $Nout $NOCONST 
    mv ga_output $ga_output_step_file
    cp $split_bb_output_step/*.patt $ga_output_step_file
    
    ga_output_n=`ls -l $ga_output_step_file | grep -c "patt"`

    if [ $ga_output_n == 0 ]; then
        echo "ga output is empty in step $step, skip following steps & do merge now"

        # if step == 1, there is no subgraph grown from seed under constraints, then finish
        if [ $step == 1 ]; then
            echo "step 1 ga empty, exit"
            exit 1
        fi

        break
    fi

    mkdir mia_output
    echo "MIA..."
    ./mia ../bitcode/${BENCH}_run_rn.bc \
    $graphdata_step_file/graphdata_bb  \
    "./${ga_output_step_file}" \
    "../data/weights.txt" \
    "../data/bblist.txt"

    files=$(ls mia_output/patt_*.data)
    for path in $files
    do
        filename=${path##*/}
        mv mia_output/${filename} ${to_merge_file}/${step}_${filename}
    done 
    
    mv mia_output $mia_step_file
done

mkdir merge_output
mkdir fu_ops_data
mkdir area_ops_data
# mkdir to_debug_pass
# ./merge to_merge/ ../data/weights.txt

mkdir cp_ops_data
# ./get_cp_ops ../bitcode/${BENCH}_run_rn.bc ./to_debug_pass 
./get_cp_ops ../bitcode/${BENCH}_run_rn.bc ./to_merge

cp -r to_merge to_debug_pass

echo "Visualization..."

rm -rf mining_output
rm -rf dfg_imgs

mkdir mining_output
mkdir dfg_imgs

./visualize ../bitcode/${BENCH}_run_rn.bc to_merge ../data/weights.txt
 ./vis.sh mining_output dfg_imgs

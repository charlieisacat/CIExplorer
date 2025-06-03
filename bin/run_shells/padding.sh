rm -rf padding_output
mkdir padding_output

root=${MINING_ROOT}

./padding \
"../ddg_bb" \
"../ddg_dep" \
"../data/weights.txt" \
"../data/bblist.txt"

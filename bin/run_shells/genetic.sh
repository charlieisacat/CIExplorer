rm -rf ga_output
mkdir ga_output

./genetic \
$1 \
$2 \
$3 \
800 \
500 \
10 \
0 \
100 \
3 \
"RNK" \
"BDM" \
"P2XO" \
"DAC"  \
"0.5" \
"0.08" \
"1.0" \
"1e-6" \
"1.0" \
"0.99" \
$4 \
$5 \
50 \
$6 \
ga_output \
"../data/weights.txt"


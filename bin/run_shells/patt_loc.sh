rm -rf debug_output
mkdir debug_output

rm -rf debug_loc_output
mkdir debug_loc_output

rm -rf debug_dfg_output
mkdir debug_dfg_output

rm -rf debug_dfg_imgs
mkdir debug_dfg_imgs

./debug_info $1 ./to_debug_pass ../data/weights.txt

./vis.sh debug_dfg_output debug_dfg_imgs

mv debug_loc_output debug_output
mv debug_dfg_output debug_output
mv debug_dfg_imgs debug_output

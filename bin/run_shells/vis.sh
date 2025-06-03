#! /bin/bash
DOT_PATH=$1
PNGS_PATH=$2
rm -rf $PNGS_PATH
mkdir $PNGS_PATH
IFS="."
for dotfile in ${DOT_PATH}/*.dot
do
    dotname=`basename $dotfile`
#    echo "$dotname, $dotfile"
    read -a strarr <<<"$dotname"
#    echo ${strarr[0]}
    pngfile="$PNGS_PATH/${strarr[0]}".png
    echo " $dotfile -> $pngfile"
    dot -Tpng "$dotfile" -o "$pngfile" 
done
#!/bin/bash


# TOP
$* &
child=$!
prefix=$(date +%Y-%m-%d-%H-%M-%S)

mkdir "./logs" 2>/dev/null

while kill -0 $child 2>/dev/null
do
    pmap  ${child} | tail -n 1 >> "./logs/${prefix}-memory.log"
    top -p ${child} -b -n 1 -d "0.001" | awk 'NR>7 && NR<13 {printf "%-4s %-4s %-s\n",$11,$9,$5}' >> "./logs/${prefix}-top.log"
    sleep "0,01"
done

echo "end"
#top -b -n 1 --pid = ${child} | tail -2 | head -3

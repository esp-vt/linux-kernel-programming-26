#!/bin/bash
SIZES=(100 1000 5000 10000 50000)
OUTPUT="bench_data.txt"

echo "# N list_ins hash_ins rb_ins xa_ins list_lkp hash_lkp rb_lkp xa_lkp" > $OUTPUT

for N in "${SIZES[@]}"; do
    sudo rmmod lkp_ds 2>/dev/null
    sudo insmod lkp_ds.ko bench_size=$N
    # /proc/lkp_ds_bench에서 데이터 추출 (awk 사용)
    DATA=$(cat /proc/lkp_ds_bench | grep -E '^[0-9]+' | tr '\n' ' ')
    echo "$N $DATA" >> $OUTPUT
done

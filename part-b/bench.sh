#!/bin/bash

OUTPUT="bench_data.txt"
SIZES=(100 1000 5000 10000 50000)

echo "# N list_ins hash_ins rb_ins xa_ins list_lkp hash_lkp rb_lkp xa_lkp" > $OUTPUT

for N in "${SIZES[@]}"; do
    echo "Running N=$N..."
    sudo rmmod lkp_ds 2>/dev/null
    sudo insmod lkp_ds.ko bench_size=$N
    sleep 0.5

    # 수정된 부분: 하이픈(-)과 숫자를 모두 정확히 인식하도록 grep 수정
    STATS=$(cat /proc/lkp_ds_bench | grep -E ': [0-9]+' | awk '{print $NF}' | tr '\n' ' ')

    if [ -n "$STATS" ]; then
        echo "$N $STATS" >> $OUTPUT
    fi
done

sudo rmmod lkp_ds 2>/dev/null
echo "Done. Check $OUTPUT"

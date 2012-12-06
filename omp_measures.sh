IMAGES="100x100@24.bmp 300x300@24.bmp 1000x1000@24.bmp 2000x2000@24.bmp 4000x4000@24.bmp 8000x8000@24.bmp"
TMP=_.bmp

for i in `seq 0 1`; do
    echo OpenMP $i
    echo "---------"
    d=openmp$i
    for core in `seq 8`; do
        export OMP_NUM_THREADS=$core
        echo OMP_NUM_THREADS=$core
        f=data/$d-$core.tsv
        cd $d && make clean && make && cd ..
        rm -f $f
        for img in $IMAGES; do
            echo " $img"
            time `$d/fisheye img/$img $TMP >> $f`
            rm -f $TMP
            echo ""
            echo ""
        done
        echo ""
    done
done

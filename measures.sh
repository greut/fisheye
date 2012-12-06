#IMAGES="100x100@24.bmp 300x300@24.bmp 748x1056@24.bmp 1680x1050@24.bmp 5184x3456@24.bmp 8000x8000@24.bmp"
IMAGES="100x100@24.bmp 300x300@24.bmp 1000x1000@24.bmp 2000x2000@24.bmp 4000x4000@24.bmp 8000x8000@24.bmp"
TMP=_.bmp
# the very max
export OMP_NUM_THREADS=8

for i in `seq 0 5`; do
    echo Serial $i
    echo "---------"
    d=src/serial$i
    tsv=data/serial$i.tsv
    cd $d && make clean && make && cd ../..
    rm -f $tsv
    for img in $IMAGES; do
        echo " $img"
        time `$d/fisheye img/$img $TMP >> $sv`
        rm -f $TMP
        echo ""
        echo ""
    done
    echo ""
done

for i in `seq 0 1`; do
    echo OpenMP $i
    echo "---------"
    d=src/openmp$i
    tsv=data/openmp$i.tsv
    cd $d && make clean && make && cd ../..
    rm -f $tsv
    for img in $IMAGES; do
        echo " $img"
        time `$d/fisheye img/$img $TMP >> $tsv`
        rm -f $TMP
        echo ""
        echo ""
    done
    echo ""
done

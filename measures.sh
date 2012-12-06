#IMAGES="100x100@24.bmp 300x300@24.bmp 748x1056@24.bmp 1680x1050@24.bmp 5184x3456@24.bmp 8000x8000@24.bmp"
IMAGES="100x100@24.bmp 300x300@24.bmp 1000x1000@24.bmp 2000x2000@24.bmp 4000x4000@24.bmp 8000x8000@24.bmp"
TMP=_.bmp

for i in `seq 0 5`; do
    echo Serial $i
    echo "---------"
    d=serial$i
    cd $d && make clean && make && cd ..
    rm -f $d.tsv
    for img in $IMAGES; do
        echo " $img"
        time `$d/fisheye img/$img $TMP >> $d.tsv`
        rm -f $TMP
        echo ""
        echo ""
    done
    echo ""
done

for i in `seq 0 0`; do
    echo OpenMP $i
    echo "---------"
    d=openmp$i
    cd $d && make clean && make && cd ..
    rm -f $d.tsv
    for img in $IMAGES; do
        echo " $img"
        time `$d/fisheye img/$img $TMP >> $d.tsv`
        rm -f $TMP
        echo ""
        echo ""
    done
    echo ""
done

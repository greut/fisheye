IMAGES="100x100@24.bmp 300x300@24.bmp 748x1056@24.bmp 1680x1050@24.bmp 5184x3456@24.bmp 8000x8000@24.bmp"
TMP=_.bmp

for i in `seq 0 3`; do
    echo Serial $i
    echo "---------"
    cd serial$i && make clean && make serial && cd ..
    rm -f serial$i.tsv
    for img in $IMAGES; do
        echo " $img"
        time `serial$i/serial img/$img $TMP >> serial$i.tsv`
        rm -f $TMP
        echo ""
        echo ""
    done
    echo ""
done

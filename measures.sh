IMAGES="100x100@24.bmp 300x300@24.bmp 1680x1050@24.bmp 8000x8000@24.bmp"
TMP=_.bmp

cd $1 && make serial > /dev/null && cd ..
for img in $IMAGES; do
    $1/serial img/$img $TMP
    rm -f $TMP
done

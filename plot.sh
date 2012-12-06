#!/bin/sh

f=1000
cores=`head -1 openmp0.tsv | awk '{printf $8}'`

gnuplot <<EOF
    set terminal png
    set output "areatime_linear.png"
    set xlabel "area [$f * pixel^2]"
    set ylabel "time [sec]"
    set key left top
    set datafile separator "\t"
    plot "serial0.tsv" u (\$2/$f):5 title "serial 0" with linespoints, \\
        "serial1.tsv" u (\$2/$f):(\$4+\$5) title "serial 1" with linespoints, \\
        "serial2.tsv" u (\$2/$f):(\$4+\$5) title "serial 2" with linespoints, \\
        "serial3.tsv" u (\$2/$f):(\$4+\$5) title "serial 3" with linespoints, \\
        "serial4.tsv" u (\$2/$f):(\$4+\$5) title "serial 4" with linespoints, \\
        "serial5.tsv" u (\$2/$f):(\$4+\$5) title "serial 5" with linespoints, \\
        "openmp0.tsv" u (\$2/$f):(\$4+\$5) title "openmp 0 ($cores)" with linespoints, \\
        "openmp1.tsv" u (\$2/$f):(\$4+\$5) title "openmp 1 ($cores)" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set output "areatime_log.png"
    set xlabel "area [log($f * pixel^2)]"
    set ylabel "time [log(sec)]"
    set logscale xy
    set key left top
    set datafile separator "\t"
    plot "serial0.tsv" u (\$2/$f):5 every ::2 title "serial 0" with linespoints, \\
        "serial1.tsv" u (\$2/$f):(\$4+\$5) every ::2 title "serial 1" with linespoints, \\
        "serial2.tsv" u (\$2/$f):(\$4+\$5) every ::2 title "serial 2" with linespoints, \\
        "serial3.tsv" u (\$2/$f):(\$4+\$5) every ::2 title "serial 3" with linespoints, \\
        "serial4.tsv" u (\$2/$f):(\$4+\$5) every ::2 title "serial 4" with linespoints, \\
        "serial5.tsv" u (\$2/$f):(\$4+\$5) every ::2 title "serial 5" with linespoints, \\
        "openmp0.tsv" u (\$2/$f):(\$4+\$5) every ::2 title "openmp 0 ($cores)" with linespoints, \\
        "openmp1.tsv" u (\$2/$f):(\$4+\$5) every ::2 title "openmp 1 ($cores)" with linespoints
EOF

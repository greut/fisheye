#!/bin/sh

f=1000

gnuplot <<EOF
    set terminal png
    set output "serial.png"
    set xlabel "area [$f * pixel^2]"
    set ylabel "time [sec]"
    set key left top
    set datafile separator "\t"
    plot "serial0.tsv" u (\$2/$f):5 title "serial 0" with linespoints, \\
        "serial1.tsv" u (\$2/$f):(\$4+\$5) title "serial 1" with linespoints, \\
        "serial2.tsv" u (\$2/$f):(\$4+\$5) title "serial 2" with linespoints, \\
        "serial3.tsv" u (\$2/$f):(\$4+\$5) title "serial 3" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set output "serial_log.png"
    set xlabel "area [log($f * pixel^2)]"
    set ylabel "time [log(sec)]"
    set logscale xy
    set key left top
    set datafile separator "\t"
    plot "serial0.tsv" u (\$2/$f):5 every ::2 title "serial 0" with linespoints, \\
        "serial1.tsv" u (\$2/$f):(\$4+\$5) every ::2 title "serial 1" with linespoints, \\
        "serial2.tsv" u (\$2/$f):(\$4+\$5) every ::2 title "serial 2" with linespoints, \\
        "serial3.tsv" u (\$2/$f):(\$4+\$5) every ::2 title "serial 3" with linespoints
EOF

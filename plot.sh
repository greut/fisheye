#!/bin/sh

f=1000
data="data"
cores=`head -1 $data/openmp0.tsv | awk '{printf $8}'`

gnuplot <<EOF
    set terminal png
    set output "plots/areatime_compute_linear.png"
    set title "Area vs Computational Time"
    set xlabel "area [$f * pixel^2]"
    set ylabel "time [sec]"
    set key left top
    set datafile separator "\t"
    plot "$data/serial0.tsv" u (\$2/$f):5 title "serial 0" with linespoints, \\
        "$data/serial1.tsv" u (\$2/$f):(\$5+\$6) title "serial 1" with linespoints, \\
        "$data/serial2.tsv" u (\$2/$f):(\$5+\$6) title "serial 2" with linespoints, \\
        "$data/serial3.tsv" u (\$2/$f):(\$5+\$6) title "serial 3" with linespoints, \\
        "$data/serial4.tsv" u (\$2/$f):(\$5+\$6) title "serial 4" with linespoints, \\
        "$data/serial5.tsv" u (\$2/$f):(\$5+\$6) title "serial 5" with linespoints, \\
        "$data/openmp0.tsv" u (\$2/$f):(\$5+\$6) title "openmp 0 ($cores)" with linespoints, \\
        "$data/openmp1.tsv" u (\$2/$f):(\$5+\$6) title "openmp 1 ($cores)" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set output "plots/areatime_compute_log.png"
    set title "Area vs Computational Time in Logscale"
    set xlabel "area [log($f * pixel^2)]"
    set ylabel "time [log(sec)]"
    set logscale xy
    set key left top
    set datafile separator "\t"
    plot "$data/serial0.tsv" u (\$2/$f):5 every ::2 title "serial 0" with linespoints, \\
        "$data/serial1.tsv" u (\$2/$f):(\$5+\$6) every ::2 title "serial 1" with linespoints, \\
        "$data/serial2.tsv" u (\$2/$f):(\$5+\$6) every ::2 title "serial 2" with linespoints, \\
        "$data/serial3.tsv" u (\$2/$f):(\$5+\$6) every ::2 title "serial 3" with linespoints, \\
        "$data/serial4.tsv" u (\$2/$f):(\$5+\$6) every ::2 title "serial 4" with linespoints, \\
        "$data/serial5.tsv" u (\$2/$f):(\$5+\$6) every ::2 title "serial 5" with linespoints, \\
        "$data/openmp0.tsv" u (\$2/$f):(\$5+\$6) every ::2 title "openmp 0 ($cores)" with linespoints, \\
        "$data/openmp1.tsv" u (\$2/$f):(\$5+\$6) every ::2 title "openmp 1 ($cores)" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set output "plots/areatime_total_linear.png"
    set title "Area vs Total Time"
    set xlabel "area [$f * pixel^2]"
    set ylabel "time [sec]"
    set key left top
    set datafile separator "\t"
    plot "$data/serial0.tsv" u (\$2/$f):3 title "serial 0" with linespoints, \\
        "$data/serial1.tsv" u (\$2/$f):3 title "serial 1" with linespoints, \\
        "$data/serial2.tsv" u (\$2/$f):3 title "serial 2" with linespoints, \\
        "$data/serial3.tsv" u (\$2/$f):3 title "serial 3" with linespoints, \\
        "$data/serial4.tsv" u (\$2/$f):3 title "serial 4" with linespoints, \\
        "$data/serial5.tsv" u (\$2/$f):3 title "serial 5" with linespoints, \\
        "$data/openmp0.tsv" u (\$2/$f):3 title "openmp 0 ($cores)" with linespoints, \\
        "$data/openmp1.tsv" u (\$2/$f):3 title "openmp 1 ($cores)" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set output "plots/areatime_total_log.png"
    set title "Area vs Total Time in Logscale"
    set xlabel "area [log($f * pixel^2)]"
    set ylabel "time [log(sec)]"
    set logscale xy
    set key left top
    set datafile separator "\t"
    plot "$data/serial0.tsv" u (\$2/$f):3 every ::2 title "serial 0" with linespoints, \\
        "$data/serial1.tsv" u (\$2/$f):3 every ::2 title "serial 1" with linespoints, \\
        "$data/serial2.tsv" u (\$2/$f):3 every ::2 title "serial 2" with linespoints, \\
        "$data/serial3.tsv" u (\$2/$f):3 every ::2 title "serial 3" with linespoints, \\
        "$data/serial4.tsv" u (\$2/$f):3 every ::2 title "serial 4" with linespoints, \\
        "$data/serial5.tsv" u (\$2/$f):3 every ::2 title "serial 5" with linespoints, \\
        "$data/openmp0.tsv" u (\$2/$f):3 every ::2 title "openmp 0 ($cores)" with linespoints, \\
        "$data/openmp1.tsv" u (\$2/$f):3 every ::2 title "openmp 1 ($cores)" with linespoints
EOF

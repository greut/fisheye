#!/bin/sh

f=1000
reftime=`head -6 data/serial5.tsv | tail -1 | awk '{printf $3}'`
reftimeCompute=`head -6 data/serial5.tsv | tail -1 | awk '{printf $5 + $6}'`
cores=`head -1 data/openmp0.tsv | awk '{printf $8}'`
versions=`seq 0 1`

for ver in $versions; do
    tsv=data/openmp${ver}_all.tsv
    rm -f $tsv
    for i in `seq $cores`; do
        echo `head -6 data/openmp${ver}-$i.tsv | tail -1` >> $tsv
    done
done

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/omp_speedup_total_8000.png"
    set title "Total time speedup on the 8000x8000 picture"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:8]
    set datafile separator " "
    plot "data/openmp0_all.tsv" u 8:8 title "Serial 5 (ideal)" with linespoints, \\
         "data/openmp0_all.tsv" u 8:($reftime/\$3) title "openMP 0" with linespoints, \\
         "data/openmp1_all.tsv" u 8:($reftime/\$3) title "openMP 1" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/omp_speedup_compute_8000.png"
    set title "Comput. time speedup on the 8000x8000 picture"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:8]
    set datafile separator " "
    plot "data/openmp0_all.tsv" u 8:8 title "Serial 5 (ideal)" with linespoints, \\
         "data/openmp0_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "openMP 0" with linespoints, \\
         "data/openmp1_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "openMP 1" with linespoints
EOF

for ver in $versions; do
    rm -f data/openmp${ver}_all.tsv
done

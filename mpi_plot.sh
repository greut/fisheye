#!/bin/sh

f=1000
reftime=`head -6 data/serial4_win.tsv | tail -1 | awk '{printf $3}'`
reftimeCompute=`head -6 data/serial4_win.tsv | tail -1 | awk '{printf $5 + $6}'`
nodes=`head -1 data/mpi0.tsv | awk '{printf $8}'`
versions=`seq 0 1`

for ver in $versions; do
    tsv=data/mpi${ver}_all.tsv
    rm -f $tsv
    for i in `seq 2 $nodes`; do
        if [ -e "data/mpi${ver}-$i.tsv" ]; then
            echo `head -6 data/mpi${ver}-$i.tsv | tail -1` >> $tsv
        fi
    done
done

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/mpi_speedup_total_8000.png"
    set title "Total time speedup on the 8000x8000 picture"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:5]
    set datafile separator " "
    plot "data/mpi1_all.tsv" u 8:8 title "Serial 4 (ideal)" with linespoints, \\
         "data/mpi0_all.tsv" u 8:($reftime/\$3) title "MPI 0" with linespoints, \\
         "data/mpi1_all.tsv" u 8:($reftime/\$3) title "MPI 1" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/mpi_speedup_compute_8000.png"
    set title "Comput. time speedup on the 8000x8000 picture"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:5]
    set datafile separator " "
    plot "data/mpi1_all.tsv" u 8:8 title "Serial 4 (ideal)" with linespoints, \\
         "data/mpi0_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 0" with linespoints, \\
         "data/mpi1_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 1" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/mpi_speedup_total_8000_no_ideal.png"
    set title "Total time speedup on the 8000x8000 picture"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:5]
    set yr [0:1]
    set datafile separator " "
    plot "data/mpi0_all.tsv" u 8:($reftime/\$3) title "MPI 0" with linespoints, \\
         "data/mpi1_all.tsv" u 8:($reftime/\$3) title "MPI 1" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/mpi_speedup_compute_8000_no_ideal.png"
    set title "Comput. time speedup on the 8000x8000 picture"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:5]
    set yr [0:1]
    set datafile separator " "
    plot "data/mpi0_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 0" with linespoints, \\
         "data/mpi1_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 1" with linespoints
EOF

for ver in $versions; do
    rm -f data/mpi${ver}_all.tsv
done

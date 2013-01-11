#!/bin/sh

f=1000
reftime=`head -6 data/serial4_win.tsv | tail -1 | awk '{printf $3}'`
reftimeCompute=`head -6 data/serial4_win.tsv | tail -1 | awk '{printf $5 + $6}'`
nodes=8
versions=`seq 0 3`

for ver in $versions; do
    tsv=data/mpi${ver}_all.tsv
    tsv2=data/mpi${ver}_all_same.tsv
    rm -f $tsv
    rm -f $tsv2
    for i in `seq 2 $nodes`; do
        if [ -e "data/mpi${ver}-$i.tsv" ]; then
            echo `head -6 data/mpi${ver}-$i.tsv | tail -1` >> $tsv
        fi
        if [ -e "data/mpi${ver}-same-$i.tsv" ]; then
            echo `head -6 data/mpi${ver}-same-$i.tsv | tail -1` >> $tsv2
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
    set xr [1:$nodes]
    set datafile separator " "
    plot "data/mpi1_all.tsv" u 8:8 title "Serial 4 (ideal)" with linespoints, \\
         "data/mpi0_all.tsv" u 8:($reftime/\$3) title "MPI 0" with linespoints, \\
         "data/mpi1_all.tsv" u 8:($reftime/\$3) title "MPI 1" with linespoints, \\
         "data/mpi2_all.tsv" u 8:($reftime/\$3) title "MPI 2" with linespoints, \\
         "data/mpi3_all.tsv" u 8:($reftime/\$3) title "MPI 3" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/mpi_speedup_compute_8000.png"
    set title "Comput. time speedup on the 8000x8000 picture"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:$nodes]
    set datafile separator " "
    plot "data/mpi1_all.tsv" u 8:8 title "Serial 4 (ideal)" with linespoints, \\
         "data/mpi0_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 0" with linespoints, \\
         "data/mpi1_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 1" with linespoints, \\
         "data/mpi2_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 2" with linespoints, \\
         "data/mpi3_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 3" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/mpi_speedup_total_8000_no_ideal.png"
    set title "Total time speedup on the 8000x8000 picture"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:$nodes]
    set yr [0:1.5]
    set datafile separator " "
    plot "data/mpi0_all.tsv" u 8:($reftime/\$3) title "MPI 0" with linespoints, \\
         "data/mpi1_all.tsv" u 8:($reftime/\$3) title "MPI 1" with linespoints, \\
         "data/mpi2_all.tsv" u 8:($reftime/\$3) title "MPI 2" with linespoints, \\
         "data/mpi3_all.tsv" u 8:($reftime/\$3) title "MPI 3" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/mpi_speedup_compute_8000_no_ideal.png"
    set title "Comput. time speedup on the 8000x8000 picture"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:$nodes]
    set yr [0:1.5]
    set datafile separator " "
    plot "data/mpi0_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 0" with linespoints, \\
         "data/mpi1_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 1" with linespoints,  \\
         "data/mpi2_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 2" with linespoints, \\
         "data/mpi3_all.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 3" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/mpi_speedup_total_same_8000_relative.png"
    set title "Total time speedup on the 8000x8000 picture (relative)"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:$nodes]
    set datafile separator " "
    plot "data/mpi1_all_same.tsv" u 8:8 title "Serial 4 (ideal)" with linespoints, \\
         "data/mpi0_all_same.tsv" u 8:($reftime/\$3) title "MPI 0" with linespoints, \\
         "data/mpi1_all_same.tsv" u 8:($reftime/\$3) title "MPI 1" with linespoints, \\
         "data/mpi2_all_same.tsv" u 8:($reftime/\$3) title "MPI 2" with linespoints, \\
         "data/mpi3_all_same.tsv" u 8:($reftime/\$3) title "MPI 3" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/mpi_speedup_compute_8000_relative.png"
    set title "Comput. time speedup on the 8000x8000 picture (relative)"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:$nodes]
    set datafile separator " "
    plot "data/mpi1_all_same.tsv" u 8:8 title "Serial 4 (ideal)" with linespoints, \\
         "data/mpi0_all_same.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 0" with linespoints, \\
         "data/mpi1_all_same.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 1" with linespoints, \\
         "data/mpi2_all_same.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 2" with linespoints, \\
         "data/mpi3_all_same.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 3" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/mpi_speedup_total_8000_no_ideal_relative.png"
    set title "Total time speedup on the 8000x8000 picture (relative)"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:$nodes]
    set yr [0:1.5]
    set datafile separator " "
    plot "data/mpi0_all_same.tsv" u 8:($reftime/\$3) title "MPI 0" with linespoints, \\
         "data/mpi1_all_same.tsv" u 8:($reftime/\$3) title "MPI 1" with linespoints, \\
         "data/mpi2_all_same.tsv" u 8:($reftime/\$3) title "MPI 2" with linespoints, \\
         "data/mpi3_all_same.tsv" u 8:($reftime/\$3) title "MPI 3" with linespoints
EOF

gnuplot <<EOF
    set terminal png
    set term png enhanced font '/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf' 13
    set output "plots/mpi_speedup_compute_8000_no_ideal_relative.png"
    set title "Comput. time speedup on the 8000x8000 picture (relative)"
    set xlabel "nodes"
    set ylabel "speedup"
    set key left top
    set xr [1:$nodes]
    set yr [0:2]
    set datafile separator " "
    plot "data/mpi0_all_same.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 0" with linespoints, \\
         "data/mpi1_all_same.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 1" with linespoints,  \\
         "data/mpi2_all_same.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 2" with linespoints, \\
         "data/mpi3_all_same.tsv" u 8:($reftimeCompute/(\$5+\$6)) title "MPI 3" with linespoints
EOF



for ver in $versions; do
    rm -f data/mpi${ver}_all.tsv
    rm -f data/mpi${ver}_all_same.tsv
done

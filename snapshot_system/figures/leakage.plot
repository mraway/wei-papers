set term post eps color solid enhanced font ", 20"
set output "leakage.eps"
set boxwidth 0.9 absolute
set key reverse top #Left outside
set yrange [0:0.06]
set ylabel "Leakage ratio (%)"
set xlabel "Number of snapshot deletions"
plot "leakage.txt" using 1:($2*100) ti 'Experiment' w l, \
'' using 1:($3*100/2) ti 'Theoretical' w l

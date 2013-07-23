set term post eps color solid enhanced font ", 20"
set output "leakage.eps"
set boxwidth 0.9 absolute
set key reverse top #Left outside
set ylabel "Leakage (%)"
set xlabel "Number of snapshot deletions"
plot "leakage.txt" using 1:2 w l

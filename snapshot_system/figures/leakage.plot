set term post eps color solid enhanced font ", 20"
set output "leakage.eps"
set boxwidth 0.9 absolute
#set key reverse top #right #outside
set key top left
set yrange [0:0.4]
set ylabel "Leakage ratio (%)"
set xlabel "Number of snapshot deletions"
plot "leakage.txt" using 1:($2*100) ti 'Estimation {/Symbol D}u=0.050U' w lp pt 5, \
'' using 1:($3*100) ti '{/Symbol D}u=0.037U' w lp pt 6, \
'' using 1:($4*100) ti '{/Symbol D}u=0.056U' w lp pt 7, \
'' using 1:($5*100) ti '{/Symbol D}u=0.021U' w lp pt 4
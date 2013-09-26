set terminal postscript eps color enhanced font ", 24"
set output "leakage.eps"
set boxwidth 0.9 absolute
#set key reverse top #right #outside
set key top left
set yrange [0:0.002]
set pointsize 2
set ylabel "Leakage ratio"
set xlabel "Number of snapshot deletions"
#plot "leakage.txt" using 1:($2*100) ti 'Estimation {/Symbol D}u=0.02U' w lp pt 6 lt 9 lw 6 lc 1, \
#'' using 1:($3*100) ti 'Measured {/Symbol D}u=0.02U' w lp pt 7 lt 1 lw 6 lc 3, \
#'' using 1:($4*100) ti 'Measured {/Symbol D}u=0.05U' w lp pt 5 lt 1 lw 6 lc 3, \
#'' using 1:($5*100) ti 'Measured {/Symbol D}u=0.10U' w lp pt 9 lt 1 lw 6 lc 3
plot "leakage.txt" using 1:2 ti 'Estimation {/Symbol D}u=0.02U' w lp pt 6 lt 9 lw 6 lc 1, \
'' using 1:3 ti 'Measured {/Symbol D}u=0.02U' w lp pt 7 lt 1 lw 6 lc 3, \
'' using 1:4 ti 'Measured {/Symbol D}u=0.05U' w lp pt 5 lt 1 lw 6 lc 3, \
'' using 1:5 ti 'Measured {/Symbol D}u=0.10U' w lp pt 9 lt 1 lw 6 lc 3
set term post eps color solid enhanced font ", 20"
set output "srb_vs_vc.eps"
set boxwidth 0.5 absolute
set key reverse top #Left outside
#set key font ",48"
set style fill solid 1.00 border -1
set style histogram clustered gap 1 title  offset character 0, 0, 0
set datafile missing '-'
set style data histograms
set style histogram rowstacked
#set xtics nomirror rotate by -45
#set title "General impact of 3-level deduplication"
set yrange [0:0.15] noreverse nowriteback
set ylabel "Time (s)"
#set xlabel "Number of concurrent snapshot backup tasks per node"
set grid y
plot 'srb_vs_vc.txt' using 2:xtic(1) ti "Data read/write" fs pattern 2, \
'' u 3 ti "Network cost" fs pattern 3, \
'' u 4 ti "Index access/lookup" fs pattern 4

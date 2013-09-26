set term post eps color solid enhanced font ", 24"
set output "vc_srb_combined.eps"
set boxwidth 0.5 absolute
set key top Left reverse #outside
#set key font ",48"
set style fill solid 1.00 border -1
set style histogram clustered gap 1 title  offset character 0, 0, 0
set datafile missing '-'
set style data histograms
set style histogram rowstacked
#set xtics nomirror rotate by -45
#set title "General impact of 3-level deduplication"
set yrange [0:0.14] noreverse nowriteback
set ylabel "Time (s)"
#set xlabel "Number of concurrent snapshot backup tasks per node"
set grid y
plot 'vc_srb_combined.txt' using 2:xtic(1) ti "Snapshot read/write" fs pattern 2, \
'' u 3 ti "Network transfer" fs pattern 3, \
'' u 4 ti "Index access and comparison" fs pattern 4

set term post eps color solid enhanced font ", 25"
set output "mem_time.eps"
set boxwidth 0.9 absolute
set key reverse Left# outside
#set key font ",48"
set style fill solid 1.00 border -1
set style histogram clustered gap 1 title  offset character 0, 0, 0
set datafile missing '-'
set style data histograms
set style histogram rowstacked
#set xtics nomirror rotate by -45
#set title "General impact of 3-level deduplication"
set yrange [0:4] noreverse nowriteback
set ylabel "Time (hour)"
set grid y
plot 'mem_time_new.data' using 3:xtic(1) ti "Stage1" fs pattern 2, \
'' u 4 ti "Stage2" fs pattern 7, \
'' u 5 ti "Stage3" fs pattern 4

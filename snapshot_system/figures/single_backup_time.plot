set term post eps color solid enhanced font ", 20"
set output "single_backup_time.eps"
set boxwidth 0.8 absolute
set key reverse top #Left outside
#set key font ",48"
set style fill solid 1.00 border -1
set style histogram clustered gap 1 title  offset character 0, 0, 0
set datafile missing '-'
set style data histograms
set style histogram rowstacked
#set xtics nomirror rotate by -45
#set title "General impact of 3-level deduplication"
set yrange [0:320] noreverse nowriteback
set ylabel "Time (sec)"
set xlabel "Size of PDS over unique data in percentage (%)"
set grid y
plot 'single_backup_time.txt' using 7:xtic(1) ti "Read" fs pattern 2, \
'' u 6 ti "Write" fs pattern 7, \
'' u ($4+$5) ti "Dedup" fs pattern 5

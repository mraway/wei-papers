set term post eps color solid enhanced font ", 20"
set output "parallel_backup_no_read.eps"
set boxwidth 0.9 absolute
set key reverse top #Left outside
#set key font ",48"
set style fill solid 1.00 border -1
set style histogram clustered gap 1 title  offset character 0, 0, 0
set datafile missing '-'
set style data histograms
#set style histogram rowstacked
#set xtics nomirror rotate by -45
set yrange [0:9000] noreverse nowriteback
#set ylabel "Deduplication and backup system throughput (MB/sec)"
set ylabel "Throughput (MB/sec)"
set xlabel "Number of concurrent snapshot backup tasks per node"
set grid y
plot 'parallel_backup_no_read.txt' using 2:xtic(1) ti "Raw" fs pattern 2, \
'' u 3 ti "Dedup" fs pattern 3, \
'' u 4 ti "QFS" fs pattern 4

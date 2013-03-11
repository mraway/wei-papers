MAIN = main
TEX = main.tex abs.tex background.tex arch.tex eval.tex concl.tex intro.tex steps.pdf snapshotdata.pdf mem_time.pdf
FIG = mem_time.pdf
BIB = dedup1.bib library.bib

all: ${MAIN}.pdf 

${MAIN}.pdf: ${TEX} ${FIG} ${BIB} makefile
	pdflatex ${MAIN}
	bibtex ${MAIN}
	pdflatex ${MAIN}
	pdflatex ${MAIN}
	#dvips -t letter -T 8.5in,11in -o ${MAIN}.ps ${MAIN}.dvi
	#ps2pdf ${MAIN}.ps

${FIG}: mem_time_new.plot mem_time_new.data
	gnuplot mem_time_new.plot
	epstopdf mem_time.eps

clean:
	rm -rf *.out *.aux *.log *.blg *.bbl main.pdf *.dvi *.ps ${MAIN}

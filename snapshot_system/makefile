MAIN	= main
TEX	= ${MAIN}.tex abs.tex intro.tex design.tex snapshot.tex append.tex cds.tex failover.tex result.tex delete.tex con.tex
FIG	= 
BIB	= library.bib

all: ${MAIN}.pdf 


${MAIN}.pdf: ${TEX} ${FIG} ${BIB} makefile
	latex ${MAIN}
	bibtex ${MAIN}
	latex ${MAIN}
	latex ${MAIN}
	dvips -t letter -T 8.5in,11in -o ${MAIN}.ps ${MAIN}.dvi
	ps2pdf ${MAIN}.ps

clean:
	rm -rf *.out *.aux *.log *.blg *.bbl *.pdf *.dvi *.ps ${MAIN}

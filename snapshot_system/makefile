MAIN	= main
TEX	= ${MAIN}.tex arch.tex abs.tex intro.tex design.tex snapshot.tex append.tex cds.tex failover.tex result.tex delete.tex concl.tex background.tex challenge.tex eval1.tex
FIG	= 
BIB	= library.bib

all: ${MAIN}.pdf 

.PHONY:remake
remake: 
	pdflatex ${MAIN}

.PHONY:bibtex
bibtex:
	bibtex ${MAIN}

${MAIN}.pdf: ${TEX} ${FIG} ${BIB} makefile
	pdflatex ${MAIN}.tex
	bibtex ${MAIN}
	pdflatex ${MAIN}.tex

clean:
	rm -rf *.out *.aux *.log *.blg *.bbl *.dvi *.ps ${MAIN}.pdf

MAIN	= main
TEX	= ${MAIN}.tex arch.tex abs.tex intro.tex design.tex snapshot.tex append.tex cds.tex failover.tex result.tex delete.tex con.tex
FIG	= 
BIB	= library.bib

all: ${MAIN}.pdf 


${MAIN}.pdf: ${TEX} ${FIG} ${BIB} makefile
	pdflatex ${MAIN}.tex
clean:
	rm -rf *.out *.aux *.log *.blg *.bbl *.dvi *.ps ${MAIN}.pdf

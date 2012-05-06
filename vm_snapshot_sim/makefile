MAIN = main
TEX = ${MAIN}.tex 2012eval.tex 2012intro.tex 2012arch.tex  2012abs.tex 2012background.tex 2012concl.tex
FIG = 
BIB = library.bib dedup1.bib

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


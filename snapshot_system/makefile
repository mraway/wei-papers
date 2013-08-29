MAIN	= fast14_main
TEX	= ${MAIN}.tex fast14_abstract.tex fast14_introduction.tex fast14_background.tex fast14_deduplication.tex fast14_architecture.tex fast14_evaluation.tex fast14_conclusion.tex
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

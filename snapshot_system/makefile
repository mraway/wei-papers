MAIN	= paper
TEX	= ${MAIN}.tex paper_abs.tex paper_analysis.tex paper_arch.tex paper_background.tex paper_conc.tex paper_dedup.tex paper_del.tex paper_eval.tex paper_intro.tex
FIG	= 
BIB	= library.bib dedup1.bib

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

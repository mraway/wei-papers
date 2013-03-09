MAIN = main
TEX = main.tex abs.tex background.tex arch.tex eval.tex 2012concl.tex
FIG = 
BIB = dedup1.bib library.bib

all: ${MAIN}.pdf 


${MAIN}.pdf: ${TEX} ${FIG} ${BIB} makefile
	pdflatex ${MAIN}
	bibtex ${MAIN}
	pdflatex ${MAIN}
	pdflatex ${MAIN}
	#dvips -t letter -T 8.5in,11in -o ${MAIN}.ps ${MAIN}.dvi
	#ps2pdf ${MAIN}.ps


clean:
	rm -rf *.out *.aux *.log *.blg *.bbl *.pdf *.dvi *.ps ${MAIN}


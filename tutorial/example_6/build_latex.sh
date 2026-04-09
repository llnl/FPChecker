#!/bin/bash 

rm -rf *.log *.aux *.out *.toc

pdflatex example6_math_formulas.tex
pdflatex example6_math_formulas.tex
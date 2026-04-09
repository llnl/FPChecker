#!/bin/bash 

pdflatex example6_math_formulas.tex
pdflatex example6_math_formulas.tex

# Keep PDF only
rm -rf *.log *.aux *.out *.toc
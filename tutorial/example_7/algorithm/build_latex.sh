#!/bin/bash 

pdflatex main.tex
pdflatex main.tex

# Keep PDF only
rm -rf *.log *.aux *.out *.toc

#!/bin/bash 

pdflatex algorithms.tex
pdflatex algorithms.tex

# Keep PDF only
rm -rf *.log *.aux *.out *.toc
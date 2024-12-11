
SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

main_tex ?= paper

VERSION_STRING := $(shell git describe --tags --long --dirty)

all: ${main_tex}.pdf

%.pdf: %.tex paper.bib version.tex
	pdflatex -shell-escape $<
	biber $* 
	pdflatex -shell-escape $<

version.tex: ${main_tex}.tex paper.bib
	echo "${VERSION_STRING}" > $@

.PHONY: clean
clean:
	rm -f ${main_tex}.aux ${main_tex}.bbl ${main_tex}.bcf ${main_tex}.log ${main_tex}.blg
	rm -f ${main_tex}.out ${main_tex}.run.xml texput.log 


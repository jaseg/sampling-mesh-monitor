
SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

main_tex ?= paper

VERSION_STRING := $(shell git describe --tags --long)

all: ${main_tex}.pdf

%.pdf: %.tex paper.bib version.tex
	pdflatex -shell-escape $<
	biber $* 
	pdflatex -shell-escape $<

version.tex: ${main_tex}.tex paper.bib
	echo "${VERSION_STRING}" > $@

paper-final.zip: paper.pdf
	rm -f paper-final.zip
	zip -r paper-final.zip \
		paper.pdf \
		paper.tex \
		version.tex \
		paper.bib \
		Makefile \
		pic_board_setup_2_small.jpg \
		block_diagram.pdf \
		pic_74lvc_small.jpg \
		pic_max3748_small.jpg \
		pic_tdp0604_small.jpg \
		pic_pi3hdx_small.jpg \
		fig_spec_risetime_74lvc.pdf \
		fig_spec_risetime_max3748.pdf \
		fig_spec_risetime_tdp0604.pdf \
		fig_spec_risetime_pi3hdx.pdf \
		fig_edge_risetime.pdf \
		fig_mesh_length.pdf \
		fig_covar_distinguish_layouts.pdf \
		fig_covar_distinguish_copies_large_run.pdf \
		fig_covar_open_p0.3.pdf \
		fig_covar_short_across_traces_p0.3.pdf \
		fig_covar_open_p0.4.pdf \
		fig_covar_short_across_traces_p0.4.pdf \
		fig_covar_short_within_0.3.pdf \
		fig_covar_probe_0.3.pdf \
		fig_covar_soldering_p0.3.pdf \
		fig_covar_antenna_wire_30mm_p0.3.pdf \
		fig_covar_patch_interleave_baseline.pdf \
		fig_covar_patch_ref_exp_interleave_direct.pdf \
		fig_patch_interleave_scatter.pdf \
		fig_drill_mod_shape_new.pdf \
		pic_manip_microsoldering_new_small.jpg \
		fig_covar_patch_repeat_p0.3.pdf \
		fig_covar_patch_repeat_p0.3_minmax.pdf \
		fig_covar_time_drift.pdf \
		fig_covar_touch_combined.pdf \
		fig_covar_hot_mesh.pdf \
		fig_tempco_edited.pdf \
		fig_covar_patch_repeat_tridelta_all_the_data_p0.3.pdf \
		fig_covar_probe_points_p0.3.pdf

.PHONY: clean
clean:
	rm -f ${main_tex}.aux ${main_tex}.bbl ${main_tex}.bcf ${main_tex}.log ${main_tex}.blg
	rm -f ${main_tex}.out ${main_tex}.run.xml texput.log 


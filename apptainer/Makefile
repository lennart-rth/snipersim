# Makefile -- build/run .sif images with Apptainer

UBUNTU_VERSION?=24.04
DEF_FILE?=Singularity-ubuntu-$(UBUNTU_VERSION).def
SIF_IMAGE?=ubuntu-$(UBUNTU_VERSION)-sniper-$(USER).sif
APPTAINER?=apptainer
APPTAINER_BUILD_OPT?=     # e.g. --sandbox or --force

# default target
all: $(SIF_IMAGE)

# build rule: .sif from .def
$(SIF_IMAGE): $(DEF_FILE)
	$(APPTAINER) build $(APPTAINER_BUILD_OPT) $@ $<

# convenience: build all Singularity*.def files in repo
DEF_FILES := $(wildcard Singularity*.def)
BUILD_ALL_TARGETS := $(patsubst %.def,%.sif,$(DEF_FILES))
build-all: $(BUILD_ALL_TARGETS)

# run as root-like shell (in container user is still current user; this mimics interactive root shell)
run-root:
	$(APPTAINER) exec $(SIF_IMAGE) /bin/bash

# run as the calling user, bind $HOME, and set working directory to current PWD
run:
	$(APPTAINER) exec --bind ${HOME}:${HOME} --pwd ${PWD} $(SIF_IMAGE) /bin/bash

.PHONY: all build-all run-root run


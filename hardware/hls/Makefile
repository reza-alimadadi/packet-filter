#!/bin/bash

VITIS=vitis_hls

ROOT_DIR=$(shell pwd)
SCRIPT=$(shell find $(ROOT_DIR) -type f -name "*.tcl")
SOLUTIONS=$(patsubst $(ROOT_DIR)/%.tcl,%,$(SCRIPT))

CLEAN ?= 0
BUILD ?= 1
REBUILD ?= 0
SYNTH ?= 1
EXPORT ?= 1

ifeq ($(CLEAN), 1)
	BUILD = 0
	SYNTH = 0
	EXPORT = 0
endif

ifeq ($(REBUILD), 1)
	CLEAN = 1
	BUILD = 1
	SYNTH = 1
	EXPORT = 1
endif

.PHONY: all
all:
	@ for solution in $(SOLUTIONS); do \
		make $$solution BUILD=1 SYNTH=1 EXPORT=1; \
	done

.PHONY: new
new:
	@ for solution in $(SOLUTIONS); do \
		make $$solution REBUILD=1; \
	done

.PHONY: setup
setup:
	@ rm -rf ./settings.tcl
	@ echo 'set clean $(CLEAN)' >> ./settings.tcl
	@ echo 'set build $(BUILD)' >> ./settings.tcl
	@ echo 'set synth $(SYNTH)' >> ./settings.tcl
	@ echo 'set export $(EXPORT)' >> ./settings.tcl

$(SOLUTIONS): % : $(ROOT_DIR)/%.tcl setup
	$(VITIS) -i -f $<;

.PHONY: clean
clean:
	@ for solution in $(SOLUTIONS); do \
		make $$solution CLEAN=1; \
	done
	rm -rf ./settings.tcl
	rm -rf ./vitis_hls.log

.PHONY: help
help:
	@ echo "Usage: make [target] [options]"
	@ echo "  e.g. make raft BUILD=1 SYNTH=1 EXPORT=1"
	@ echo "Targets:"
	@ echo "  all:     Build all solutions"
	@ echo "  new:     Clean and rebuild all solutions"
	@ for solution in $(SOLUTIONS); do \
		echo "  $$solution:    Build $$solution with following options"; \
	done
	@ echo "  clean:   Clean all solutions and remove log files"
	@ echo "Options:"
	@ echo "  CLEAN:   Clean the build directory (default=0)"
	@ echo "  BUILD:   Build the solution (default=1)"
	@ echo "  REBUILD: Clean and rebuild the solution (default=0)"
	@ echo "  SYNTH:   Synthesize the solution (default=1)"
	@ echo "  EXPORT:  Export the solution (default=1)"

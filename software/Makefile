
NPROC ?= $(shell nproc)

.PHONY: all
all: build

.PHONY: build
build:
	rm -rf build/CMakeCache.txt
	cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
	make -C build -j $(NPROC)

.PHONY: debug
debug:
	rm -rf build/CMakeCache.txt
	cmake -B build -DCMAKE_BUILD_TYPE=Debug
	make -C build -j $(NPROC)

.PHONY: clean
clean:
	rm -rf build

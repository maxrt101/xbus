# xbus

export TOPDIR   := $(shell pwd)
export BUILD    := $(TOPDIR)/build
export SHELL    := /bin/bash -e
export CXX      := g++
export AR       := ar
export CXXFLAGS := -std=c++17 -I$(BUILD)/include

MRTURL := https://github.com/maxrt101/mrt.git
LIB    := $(BUILD)/lib/libxbus.a
SRC    := src/lib/*.cc

ifeq ($(V),2)
SHELL := /bin/bash -ex
endif

ifneq ($(DEBUG),0)
CXXFLAGS += -g3 -D_DEBUG
endif

define Compile
	for file in $(1); do \
		$(CXX) $(CXXFLAGS) -c $$file -o "$(BUILD)/obj/$(2)$$(basename $${file%.*}).o"; \
	done
endef

.PHONY: build

build: lib
	$(info [+] Building xbus/xbusd)
	$(CXX) $(CXXFLAGS) -L$(BUILD)/lib -lxbus src/xbusd.cc -o $(BUILD)/bin/xbusd
	$(CXX) $(CXXFLAGS) -L$(BUILD)/lib -lxbus src/xbus.cc -o $(BUILD)/bin/xbus

lib: install-headers
	$(info [+] Building libff)
	rm -rf $(LIB)
	rm -rf $(BUILD)/obj/*
	$(call Compile,$(SRC))
	$(AR) cr $(LIB) $(BUILD)/obj/*.o

install-headers: dependencies
	$(info [+] Installing headers)
	cp -r include $(BUILD)/include/xbus

dependencies: prepare
	if [[ ! -d $(BUILD)/include/mrt ]] || [[ "$(FORCE)" == "1" ]]; then \
	 	echo "[+] Installing dependencies"; \
		if [[ ! -d mrt ]]; then \
			git clone $(MRTURL); \
		fi; \
		cd mrt; \
		make PREFIX=$(BUILD); \
	else \
		echo "[+] Dependencies already satisfied"; \
	fi

prepare:
	$(info [+] Creating folders)
	mkdir -p $(BUILD)
	mkdir -p $(BUILD)/bin
	mkdir -p $(BUILD)/include
	mkdir -p $(BUILD)/lib
	mkdir -p $(BUILD)/obj
	rm -rf $(BUILD)/include/xbus

clean:
	$(info [+] Cleaning build)
	$(RM) -rf $(BUILD)

install: build
	if [[ "$(PREFIX)" == "" ]]; then \
		echo [-] PREFIX must be defined; \
	else \
		cp -r $(BUILD)/include/* $(PREFIX)/include; \
		cp $(LIB) $(PREFIX)/lib; \
		cp $(BUILD)/bin/xbus $(PREFIX)/bin; \
		cp $(BUILD)/bin/xbusd $(PREFIX)/bin; \
	fi

test:
	$(info [+] Building test)
	$(CXX) $(CXXFLAGS) -Lbuild/lib -lxbus src/test.cc -o $(BUILD)/bin/test

$(V).SILENT:

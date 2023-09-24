SHELL := /bin/bash

#Compiler and flag settings
CC=gcc			#GCC Compiler is the default
CFLAGS=-g		#Build with debugging enabled by default

#HELP
.PHONY: help
help:
	@echo "Usage make <TARGET>"
	@echo ""
	@echo "  Targets:"
	@echo "	   build				Build the decoder executable"
	@echo "	   run					Run the decoder program"

.PHONY: build
build: *.c *.h
	$(CC) $(CFLAGS) -o decoder *.c 

.PHONY: run
run: decoder
	./decoder
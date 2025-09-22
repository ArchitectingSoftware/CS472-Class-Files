# Makefile for UDP Echo Program

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = udp-echo
SOURCE = udp-echo.c

# Default target
all: $(TARGET)

# Build the program
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Install (copy to /usr/local/bin)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Run server for testing
run-server: $(TARGET)
	./$(TARGET) --server

# Run client for testing
run-client: $(TARGET)
	./$(TARGET) --client

# Debug with gdb (server)
debug-server: $(TARGET)
	gdb --args ./$(TARGET) --server

# Debug with gdb (client)
debug-client: $(TARGET)
	gdb --args ./$(TARGET) --client

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build the program (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install to /usr/local/bin"
	@echo "  uninstall    - Remove from /usr/local/bin"
	@echo "  run-server   - Build and run server"
	@echo "  run-client   - Build and run client"
	@echo "  debug-server - Build and run server in gdb"
	@echo "  debug-client - Build and run client in gdb"
	@echo "  help         - Show this help message"

# Declare phony targets
.PHONY: all clean install uninstall run-server run-client debug-server debug-client help
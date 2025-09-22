# Makefile for TCP Echo Program

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra  -g
TARGET = tcp-echo
SOURCE = tcp-echo.c

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

# Test with custom port
test-server: $(TARGET)
	./$(TARGET) --server --port 8080

test-client: $(TARGET)
	./$(TARGET) --client --port 8080

# Test server shutdown command (useful for demos)
test-exit-server: $(TARGET)
	@echo "This will connect and send 'exit server' command:"
	@echo "exit server" | ./$(TARGET) --client

# Show help
help:
	@echo "Available targets:"
	@echo "  all             - Build the program (default)"
	@echo "  clean           - Remove build artifacts"
	@echo "  install         - Install to /usr/local/bin"
	@echo "  uninstall       - Remove from /usr/local/bin"
	@echo "  run-server      - Build and run server"
	@echo "  run-client      - Build and run client (interactive)"
	@echo "  test-server     - Build and run server on port 8080"
	@echo "  test-client     - Build and run client connecting to port 8080"
	@echo "  test-exit-server - Build and test server shutdown command"
	@echo "  debug-server    - Build and run server in gdb"
	@echo "  debug-client    - Build and run client in gdb"
	@echo "  help            - Show this help message"
	@echo ""
	@echo "Usage Examples:"
	@echo "  make run-server    # Start server in terminal 1"
	@echo "  make run-client    # Start interactive client in terminal 2"
	@echo "  make test-exit-server  # Test server shutdown (server must be running)"

# Declare phony targets
.PHONY: all clean install uninstall run-server run-client test-server test-client test-exit-server debug-server debug-client help
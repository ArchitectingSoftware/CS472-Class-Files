# Makefile for TCP Echo Program

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -gdwarf-4 -O0  -g
TARGET = crypto-echo
SOURCE = crypto-echo.c crypto-lib.c crypto-client.c crypto-server.c


# Default target
all: $(TARGET) $(HELPER)

# Build the program
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)


# Clean build artifacts
clean:
	rm -f $(TARGET) 

# Run server for testing
run-server: $(TARGET)
	./$(TARGET) --server

# Run client for testing
run-client: $(TARGET)
	./$(TARGET) --client


# Show help
help:
	@echo "Available targets:"
	@echo "  all             - Build the program (default)"
	@echo "  clean           - Remove build artifacts"
	@echo "  run-server      - Build and run server"
	@echo "  run-client      - Build and run client (interactive)"
	@echo "  help            - Show this help message"
	@echo ""
	@echo "Usage Examples:"
	@echo "  make run-server    # Start server in terminal 1"
	@echo "  make run-client    # Start interactive client in terminal 2"
	@echo "  make test-exit-server  # Test server shutdown (server must be running)"

# Declare phony targets
.PHONY: all clean install uninstall run-server run-client test-server test-client test-exit-server debug-server debug-client help
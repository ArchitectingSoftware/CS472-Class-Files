CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = ntp-client
SOURCES = ntp-client.c
HEADERS = ntp-protocol.h

# Build without unused-variable warnings
no-warn: CFLAGS := -Wall -Wextra -std=c99 -g -Wno-unused-variable -Wno-unused-parameter
no-warn: $(TARGET)

# Build the NTP client
$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) 

# Simple test
test: $(TARGET)
	@echo "Testing NTP client..."
	./$(TARGET)

# Test with specific server
test-server: $(TARGET)
	@echo "Testing with time.nist.gov..."
	./$(TARGET) -s time.nist.gov
	@echo ""
	@echo "Testing with pool.ntp.org..."
	./$(TARGET) -s pool.ntp.org

# Clean up
clean:
	rm -f $(TARGET) *.o

# Check struct sizes (educational)
check-structs: $(TARGET)
	@echo "=== NTP Structure Sizes ==="
	@echo "Checking packed struct sizes..."
	@echo '#include <stdio.h>' > struct_sizes.c
	@echo '#include "ntp-protocol.h"' >> struct_sizes.c
	@echo 'int main() {' >> struct_sizes.c
	@echo '  printf("ntp_timestamp_t: %zu bytes\\n", sizeof(ntp_timestamp_t));' >> struct_sizes.c
	@echo '  printf("ntp_packet_t: %zu bytes\\n", sizeof(ntp_packet_t));' >> struct_sizes.c
	@echo '  printf("Expected NTP packet size: 48 bytes\\n");' >> struct_sizes.c
	@echo '  if (sizeof(ntp_packet_t) == 48) printf("✓ Struct packing correct!\\n");' >> struct_sizes.c
	@echo '  else printf("✗ Struct packing incorrect!\\n");' >> struct_sizes.c
	@echo '  return 0; }' >> struct_sizes.c
	@$(CC) $(CFLAGS) -o struct_sizes struct_sizes.c
	@./struct_sizes
	@rm -f struct_sizes struct_sizes.c

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build the NTP client (default)"
	@echo "  test         - Test with default NTP server"
	@echo "  test-server  - Test with different servers"
	@echo "  check-structs- Verify struct sizes"
	@echo "  clean        - Remove built files"

# Default target
all: $(TARGET)

.PHONY: all test test-server check-structs clean help
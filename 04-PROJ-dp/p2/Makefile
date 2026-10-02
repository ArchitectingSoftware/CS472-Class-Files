CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

all: dp-client dp-server dp-log

dp-client: dp-client.o dp-proto.o
	$(CC) $(CFLAGS) -o $@ $^

dp-server: dp-server.o dp-proto.o
	$(CC) $(CFLAGS) -o $@ $^

dp-log: dp-log.o
	$(CC) $(CFLAGS) -o $@ $^

dp-client.o: dp-client.c dp-proto.h dp-app.h
	$(CC) $(CFLAGS) -c $<

dp-server.o: dp-server.c dp-proto.h dp-app.h
	$(CC) $(CFLAGS) -c $<

dp-proto.o: dp-proto.c dp-proto.h
	$(CC) $(CFLAGS) -c $<

dp-log.o: dp-log.c
	$(CC) $(CFLAGS) -c $<

test: all
	./test.sh

hol: all
	./hol.sh

evidence: all
	@if [ -z "$(ID)" ]; then echo "usage: make evidence ID=your-drexel-id"; exit 1; fi
	./evidence.sh $(ID)

merge: dp-log
	@found=0; \
	for client_log in client-*.log; do \
		[ -f "$$client_log" ] || continue; \
		label=$${client_log#client-}; \
		label=$${label%.log}; \
		server_log="server-$$label.log"; \
		if [ -f "$$server_log" ]; then \
			echo "merge: $$client_log + $$server_log -> merged-$$label.log"; \
			./dp-log "$$client_log" "$$server_log" > "merged-$$label.log"; \
			found=1; \
		else \
			echo "merge: missing $$server_log" >&2; \
		fi; \
	done; \
	if [ "$$found" -eq 0 ]; then echo "merge: no matching client/server log pairs found"; exit 1; fi

clean:
	rm -f dp-client dp-server dp-log *.o client-*.log server-*.log merged-*.log hol-*.log hol-results.txt

.PHONY: all test hol evidence merge clean

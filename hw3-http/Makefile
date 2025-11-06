all: client-cc client-ka

client-cc: client-cc.c http.c
	gcc -g client-cc.c http.c -o client-cc

client-ka: client-ka.c http.c
	gcc -g client-ka.c http.c -o client-ka

.PHONY: run-cc
run-cc:
	./client-cc httpbin.org 80 /

.PHONY: run-cc3
run-cc3:
	./client-cc httpbin.org 80 / /json /html

.PHONY: run-ka
run-ka:
	./client-ka httpbin.org 80 /

.PHONY: run-ka3
run-ka3:
	./client-ka httpbin.org 80 / /json /html

.PHONY: run-cc-cci
run-cc-cci:
	./client-cc cci-p141.cci.drexel.edu 80 /

.PHONY: run-cc3-cci
run-cc3-cci:
	./client-cc cci-p141.cci.drexel.edu 80 / /json /html

.PHONY: run-ka-cci
run-ka-cci:
	./client-ka cci-p141.cci.drexel.edu 80 /

.PHONY: run-ka3-cci
run-ka3-cci:
	./client-ka cci-p141.cci.drexel.edu 80 / /json /html

.PHONY: clean
clean:
	rm -f client-cc client-ka
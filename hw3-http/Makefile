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

.PHONY: run-cc-aws
run-cc-aws:
	./client-cc ec2-98-94-0-172.compute-1.amazonaws.com 80 /

.PHONY: run-cc3-aws
run-cc3-aws:
	./client-cc ec2-98-94-0-172.compute-1.amazonaws.com 80 / /json /html

.PHONY: run-ka-aws
run-ka-aws:
	./client-ka ec2-98-94-0-172.compute-1.amazonaws.com 80 /

.PHONY: run-ka3-aws
run-ka3-aws:
	./client-ka ec2-98-94-0-172.compute-1.amazonaws.com 80 / /json /html

.PHONY: clean
clean:
	rm -f client-cc client-ka
####### Make file
# Zeyu Li
# ID: 8349389776
# Use <make all> to compile all the five c-files
####### How to run:
# Open five terminal, run each program in the following order on different terminal
# 1. run two backend servers in any order use <make server#>
# 2. run aws use <make aws>
# 3. run monitor <make monitor>
# 4. run client use <./client write BW LENGTH VELOCITY NOISE> or <./client compute LINK_ID SIZE POWER>
# 5. client.c will terminate by itself, other programs need to use <crtl+c> to terminate.
#######

all: 
	gcc -o awsoutput aws.c
	gcc -o client client.c
	gcc -o serverAoutput serverA.c
	gcc -o serverBoutput serverB.c
	gcc -o monitoroutput monitor.c

.PHONY: aws
aws:
	./awsoutput

.PHONY:serverA
serverA:
	./serverAoutput

.PHONY:serverB
serverB:
	./serverBoutput

.PHONY: monitor
monitor:
	./monitoroutput


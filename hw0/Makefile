all: wc.c main.c map.c
	gcc -std=c99 -g -Wall -o wc wc.c -I.
	gcc -std=c99 -g -Wall -o main main.c -I. 
	gcc -std=c99 -g -Wall -o map map.c -I.
	gcc -std=c99 -g -Wall -o wc-test wc-test.c -I.

clean: 
	$(RM) wc main map

check:
	./wc-test 

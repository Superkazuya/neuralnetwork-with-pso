.PHONY: all
all : ps main
ps : ps.c
	gcc $^ -o $@ -lm

main : main.c
	gcc $^ -o $@ -I/usr/include/libxml2 -lm -lxml2
.PHONY: all
all : ps.out nn.out
ps.out : ps.c
	gcc $^ -o $@ -lm -pthread

nn.out : main.c
	gcc $^ -o $@ -I/usr/include/libxml2 -lm -lxml2

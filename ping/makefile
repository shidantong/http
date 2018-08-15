.PHONY:all clean

all:ping

ping:ping.o
	gcc $^ -o $@

%.o:%.c
	gcc -c $^ -o $@

clean: 
	rm *.o


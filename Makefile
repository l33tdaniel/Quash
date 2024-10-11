STUDENT_ID_1 = 3098379
STUDENT_ID_2 = 3098379

CCFLAGS = -Wall -std=gnu11

all: quash 

quash: quash.c
	gcc $(CCFAGS) -g -o $@ $^ 

clean:
	rm -rf quash 

test: all


# Makefile for UD CISC user-level thread library

CC = gcc
CFLAGS = -g

LIBOBJS = t_lib.o 

TSTOBJS = test00.o 

# specify the executable 

EXECS = test00

# specify the source files

LIBSRCS = t_lib.c

TSTSRCS = test00.c

# ar creates the static thread library

t_lib.a: ${LIBOBJS} Makefile
	ar rcs t_lib.a ${LIBOBJS}

# here, we specify how each file should be compiled, what
# files they depend on, etc.

t_lib.o: t_lib.c t_lib.h Makefile
	${CC} ${CFLAGS} -c t_lib.c

test00.o: test00.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test00.c

test00: test00.o t_lib.a Makefile
	${CC} ${CFLAGS} test00.o t_lib.a -o test00
	
test01.o: test01.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test01.c

test01: test01.o t_lib.a Makefile
	${CC} ${CFLAGS} test01.o t_lib.a -o test01	

test01x.o: test01x.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test01x.c

test01x: test01x.o t_lib.a Makefile
	${CC} ${CFLAGS} test01x.o t_lib.a -o test01x

test01a.o: test01a.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test01a.c

test01a: test01a.o t_lib.a Makefile
	${CC} ${CFLAGS} test01a.o t_lib.a -o test01a

test03.o: test03.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test03.c

test03: test03.o t_lib.a Makefile
	${CC} ${CFLAGS} test03.o t_lib.a -o test03

test10.o: test10.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test10.c

test10: test10.o t_lib.a Makefile
	${CC} ${CFLAGS} test10.o t_lib.a -o test10

3test.o: 3test.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c 3test.c

3test: 3test.o t_lib.a Makefile
	${CC} ${CFLAGS} 3test.o t_lib.a -o 3test

test06.o: test06.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test06.c

test06: test06.o t_lib.a Makefile
	${CC} ${CFLAGS} test06.o t_lib.a -o test06



clean:
	rm -f t_lib.a ${EXECS} ${LIBOBJS} ${TSTOBJS} 

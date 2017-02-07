TARGET = murasaki
CC=gcc
OBJS = \
  lex.yy.o\
  y.tab.o\
  main.o\
  interface.o\
  create.o\
  execute.o\
  eval.o\
  string.o\
  heap.o\
  util.o\
  native.o\
  error.o\
  error_message.o\
  ./memory/mem.o\
  ./debug/dbg.o
CFLAGS = -c -g -Wall -Wswitch-enum -ansi -pedantic -DDEBUG
INCLUDES = \

$(TARGET):$(OBJS)
	cd ./memory; $(MAKE);
	cd ./debug; $(MAKE);
	$(CC) $(OBJS) -o $@ -lm
clean:
	rm -f *.o lex.yy.c y.tab.c y.tab.h *~
y.tab.h : murasaki.y
	bison --yacc -dv murasaki.y
y.tab.c : murasaki.y
	bison --yacc -dv murasaki.y
lex.yy.c : murasaki.l murasaki.y y.tab.h
	flex murasaki.l
y.tab.o: y.tab.c murasaki.h MEM.h
	$(CC) -c -g $*.c $(INCLUDES)
lex.yy.o: lex.yy.c murasaki.h MEM.h
	$(CC) -c -g $*.c $(INCLUDES)
.c.o:
	$(CC) $(CFLAGS) $*.c $(INCLUDES)
./memory/mem.o:
	cd ./memory; $(MAKE);
./debug/dbg.o:
	cd ./debug; $(MAKE);
############################################################
create.o: create.c MEM.h DBG.h murasaki.h MRSK.h MRSK_dev.h
error.o: error.c MEM.h murasaki.h MRSK.h MRSK_dev.h
error_message.o: error_message.c murasaki.h MEM.h MRSK.h MRSK_dev.h
eval.o: eval.c MEM.h DBG.h murasaki.h MRSK.h MRSK_dev.h
execute.o: execute.c MEM.h DBG.h murasaki.h MRSK.h MRSK_dev.h
heap.o: heap.c MEM.h DBG.h murasaki.h MRSK.h MRSK_dev.h
interface.o: interface.c MEM.h DBG.h murasaki.h MRSK.h MRSK_dev.h
main.o: main.c MRSK.h MEM.h
native.o: native.c MEM.h DBG.h murasaki.h MRSK.h MRSK_dev.h
string.o: string.c MEM.h murasaki.h MRSK.h MRSK_dev.h
util.o: util.c MEM.h DBG.h murasaki.h MRSK.h MRSK_dev.h
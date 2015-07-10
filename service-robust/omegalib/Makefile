CC = gcc
INCDIR = ../include
CFLAGS = -Wall -fPIC -I$(INCDIR) -O2 #-g
LFLAGS = -lm
OFILES = ../src/pipe.o ../src/msg.o

all:		libservice-robust.so

libservice-robust.so:	service-robustlib.o
		$(CC) $(CFLAGS) $(OFILES) $(LFLAGS) -shared -o $@ $< 
		ln -s libservice-robust.so libservice-robust.sl

service-robustlib.o:	service-robustlib.c $(INCDIR)/service-robustlib.h $(INCDIR)/omega.h $(INCDIR)/msg.h $(INCDIR)/pipe.h Makefile
		$(CC) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) -fr *.o *.so *.sl


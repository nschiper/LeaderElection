CC = gcc
INCDIR = ../include
CFLAGS = -O2 -Wall -fPIC -I$(INCDIR)
LFLAGS = -lm
OFILES = ../src/pipe.o ../src/msg.o

all:		libservice-scalable.so

libservice-scalable.so:	service-scalablelib.o
		$(CC) $(CFLAGS) $(OFILES) $(LFLAGS) -shared -o $@ $< 
		ln -s libservice-scalable.so libservice-scalable.sl

service-scalablelib.o:	service-scalablelib.c $(INCDIR)/service-scalablelib.h $(INCDIR)/omega.h $(INCDIR)/msg.h $(INCDIR)/pipe.h Makefile
		$(CC) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) -fr *.o *.so *.sl


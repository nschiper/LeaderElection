CC = gcc
INCDIR = ../include
CFLAGS = -Wall -I$(INCDIR) -O2 #-g
LFLAGS =-lpthread -lnsl -lm #-lsocket 
OFILES = service-scalable.o omega_remote.o omega_local.o omega_algorithm.o omega_fifo.o msg.o misc.o pipe.o fdd.o fdd_comm.o fdd_local.o fdd_remote.o fdd_stats.o fdd_sched.o\
variables_exchange.o fdd_wei.o fdd_spread.o fdd_initial_ed.o
DEP = $(INCDIR)/misc.h $(INCDIR)/fdd_types.h $(INCDIR)/fdd_msg.h\
$(INCDIR)/fdd.h $(INCDIR)/variables_exchange.h $(INCDIR)/list.h\
$(INCDIR)/fdd_stats.h $(INCDIR)/fdd_portab.h $(INCDIR)/omega.h Makefile
OPT_DEFINES =  -DINSTANT_EXPECTED_DELAY_OFF -DSYNCH_CLOCKS -DREALTIME #-DOUTPUT -DOMEGA_OUTPUT -DLOG -DOUTPUT_EXTRA -DOMEGA_LOG


%.o:		%.c $(DEP)
		$(CC) $(OPT_DEFINES) $(CFLAGS) -c $<

service-scalable:		$(OFILES)
		$(CC) -o $@ $(OFILES) $(LFLAGS)
		echo "Compilation completed"

clean:
		$(RM) -fr *.o service-scalable 



#include <omega1lib.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#define NIPQUAD(addr)                                   \
        (u_char)((addr)->sin_addr.s_addr),              \
        (u_char)((addr)->sin_addr.s_addr >> 8),         \
        (u_char)((addr)->sin_addr.s_addr >> 16),        \
        (u_char)((addr)->sin_addr.s_addr >> 24)

int omega_interrupt = 0;


void printChoices1() {

	printf("available commands:\n");
	printf("\ta) register process : a pid\n");
	printf("\th) exit\n");
	printf("\nenter command:\n");
}


void printChoices2() {

	printf("available commands:\n");
	printf("\tb) unregister process : b\n");
	printf("\tc) start omega : c gid candidate notif_type Tdu(ms) TmU(ms)"
	       " TmrL(ms)\n");
	printf("\td) stop omega : d gid\n");
	printf("\te) get leader : e gid\n");
	printf("\tf) change interrupt mode to ANY_CHANGE : f gid\n");
	printf("\tg) change interrupt mode to INTERRUPT_NONE : g gid\n");
	printf("\th) exit\n");
	printf("\nenter command:\n");
}


/* Parses and executes the command using, if necessary, omega_interrupt */
int execute_command1(fd_set *dset) {
	char command[100];
	char cmd;
	unsigned int pid;
	int retval = 0;

	scanf("%s", command);

	switch(command[0]) {
		case 'a' : scanf("%c %u", &cmd, &pid);
		           retval = omega_register(pid);
		           if (retval >= 0) {
		               omega_interrupt = retval;
		               FD_SET(omega_interrupt, dset);
				   }
		           return retval;
		case 'h' : exit(0);
		           break;
		default:   return -1;
	}
}


/* Parses and executes the command using, if necessary, omega_interrupt */
int execute_command2(fd_set *dset) {
	char command[100];
	char cmd;
	unsigned int gid;
	int candidate, notif_type;
	unsigned int TdU, TmU, TmrL;
	struct omega_proc_struct leader;
	int retval = 0;

	scanf("%s", command);

	switch(command[0]) {
		case 'b' : scanf("%c", &cmd);
		           retval = omega_unregister(omega_interrupt);
		           if (retval >= 0) {
		               FD_CLR(omega_interrupt, dset);
		               omega_interrupt = 0;
			       }
		           return retval;
		case 'c' : scanf("%c %u %d %d %u %u %u", &cmd, &gid, &candidate,
				          &notif_type, &TdU, &TmU, &TmrL);
				   candidate = ((candidate == 1) ?
				       CANDIDATE : NOT_CANDIDATE);
				   notif_type = ((notif_type == 1) ?
				       OMEGA_INTERRUPT_ANY_CHANGE : OMEGA_INTERRUPT_NONE);
		           retval = omega_startOmega(omega_interrupt, gid,
		               candidate, notif_type, TdU, TmU, TmrL);
		           return retval;
		case 'd' : scanf("%c %u", &cmd, &gid);
		           retval = omega_stopOmega(omega_interrupt, gid);
		           return retval;
		case 'e' : scanf("%c %u", &cmd, &gid);
		           retval = omega_getleader(omega_interrupt, gid, &leader);
		           if (retval >= 0) {
				       if (leader.leader_stable)
                           printf("Leader(stable) is pid: %u address: %u.%u.%u.%u group: %u\n",
						          leader.pid, NIPQUAD(&leader.addr), leader.gid);
				       else
					       printf("Leader(unstable) is pid: %u address: %u.%u.%u.%u group: %u\n",
						          leader.pid, NIPQUAD(&leader.addr), leader.gid);
				   }
		           return retval;
		case 'f' : scanf("%c %u", &cmd, &gid);
		           retval = omega_interrupt_any_change(omega_interrupt,
		                                               gid);
		           return retval;
		case 'g' : scanf("%c %u", &cmd, &gid);
		           retval = omega_interrupt_none(omega_interrupt, gid);
		           return retval;
		case 'h' : exit(0);
		           break;
		default:   return -1;
	}
}


int main(int argc, char *argv[]) {

	fd_set active, dset;
	struct omega_proc_struct leader;
	int retval;

	FD_ZERO(&active);
	FD_ZERO(&dset);

	FD_SET(STDIN_FILENO, &dset);

	while(1) {
		memcpy(&active, &dset, sizeof(fd_set));
		if (omega_interrupt == 0)
			printChoices1();
		else
			printChoices2();

		select(FD_SETSIZE, &active, NULL, NULL, NULL);

		if (FD_ISSET(STDIN_FILENO, &active)) {
			if (omega_interrupt == 0)
				retval = execute_command1(&dset);
			else
				retval = execute_command2(&dset);

			if (retval < 0)
				printf("Error executing command\n");
			else
				printf("command executed succesfully\n");
		}
		/* if omega_interrupt is initialized */
		if (omega_interrupt != 0) {
			if (FD_ISSET(omega_interrupt, &active)) {
				if (omega_parse_notify(omega_interrupt, &leader) < 0)
					printf("Error parsing notify\n");
				else {
					if (leader.leader_stable)
						printf("Leader(stable) is pid: %u address: %u.%u.%u.%u group: %u\n",
						        leader.pid, NIPQUAD(&leader.addr), leader.gid);
					else
						printf("Leader(unstable) is pid: %u address: %u.%u.%u.%u group: %u\n",
						        leader.pid, NIPQUAD(&leader.addr), leader.gid);
				}
			}
		}
	}
}

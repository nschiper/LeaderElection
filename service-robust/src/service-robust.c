//  This file is part of a leader election service 
//  See http://www.inf.unisi.ch/phd/schiper/LeaderElection/
//
//  Author: Daniel Ivan and Nicolas Schiper 
//  Copyright (C) 2001-2006 Daniel Ivan and Nicolas Schiper
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
//  USA, or send email to nicolas.schiper@lu.unisi.ch.
//


/* service-robust.c */

#include <stdio.h>
#include <stdlib.h>
#include "omega.h"
#include <sys/select.h>
#include <unistd.h>
#include "variables_exchange.h"
#include "omega_remote.h"
#include "misc.h"
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <pthread.h>

#ifdef OMEGA_LOG
	FILE *omega_log ;
#endif



struct sockaddr_in omega_localaddr;


void terminate_omega(int sign) {

	if (sign == SIGINT) {

		terminate_fdd();

		(void)unlink(OMEGA_FIFO_REG) ;
		#ifdef OMEGA_LOG
			fclose(omega_log);
		#endif
		close(omega_udp_socket);
		pthread_mutex_destroy(&registeredproc_list_mutex);
		exit(EXIT_SUCCESS);
	}
}

static void init_omega(fd_set *dset) {

	struct utsname uts;
	struct hostent *hst = NULL;

	/* Set the SIGINT handling */
	struct sigaction action;


	/* Lock pages in memory and change process priority */
	#ifdef REALTIME
		if (mlockall(MCL_CURRENT | MCL_FUTURE) < 0) {
			perror("Couldn't lock pages in memory");
			exit(-1);
		}
		errno = 0;
		if (setpriority(PRIO_PROCESS, 0, -20) < 0) {
			if (errno < 0) {
				perror("Couldn't change process priority");
				exit(-1);
			}
		}
	#endif



	action.sa_handler = terminate_omega ;
	action.sa_flags = 0 ;
	sigemptyset(&action.sa_mask) ;

	if(sigaction(SIGINT, &action, NULL) < 0) {
		fprintf(stderr, "Initialization failed, exiting application.\n");
		exit(-1);
	}

	/* Set the SIGPIPE handling */
	action.sa_handler = SIG_IGN ;
	if( sigaction(SIGPIPE, &action, NULL) < 0) {
		fprintf(stderr, "Initialization failed, exiting application.\n");
		exit(-1);
	}

	/* Get the local ip address */
	  /* retrive information about the current kernel and machine */
	if(uname(&uts) < 0) {
		#ifdef OMEGA_OUTPUT
			perror("omega: uname");
		#endif
	    exit(-1);
	}

	/* get the host like information about the current machine */
	hst = gethostbyname(uts.nodename);
	if (hst == NULL) {
		fprintf(stderr, "omega: could not resolve %s\n", uts.nodename);
		exit(-1);
	}

	/* the network address of the current machine. */
	omega_localaddr.sin_addr.s_addr = (*(u_long *)hst->h_addr);


	if (exchange_vars_init() < 0) {
		fprintf(stderr, "Initialization failed, exiting application.\n");
		exit(-1);
	}
	if (omega_algorithm_init() < 0) {
		fprintf(stderr, "Initialization failed, exiting application.\n");
		exit(-1);
	}
	if (pthread_mutex_init(&registeredproc_list_mutex, NULL) < 0) {
		fprintf(stderr, "Initialization failed, exiting application.\n");
		exit(-1);
	}

	#ifdef OMEGA_LOG
		if( (omega_log = fopen("omega_log", "w")) == NULL ) {
			perror("Could not make the omega_log file") ;
		}
	#endif

	omega_local_init();

	/* Open the registration pipe */
	if( (omega_fifo_fd = omega_fifo_init()) < 0 ) {
		fprintf(stderr, "omega: omega_fifo_init() failed.\n");
		exit(-1);
	}

	FD_ZERO(dset);
	FD_SET(omega_fifo_fd, dset);

	#ifdef OMEGA_OUTPUT
		FD_SET(0, dset) ;
	#endif


	/* create udp socket */
	omega_udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(omega_udp_socket < 0) {
		perror("omega: Cannot open udp socket\n");
		exit(-1);
	}

	/* bind server port */
	bzero(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(OMEGA_UDP_PORT);

	if(bind(omega_udp_socket, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
		perror("omega: cannot bind udp port\n");
		exit(-1);
	}

	FD_SET(omega_udp_socket, dset);

	return ;
}


int main(int argc, char *argv[]) {

	fd_set dset, active;
	int selected;
	int fd_udp_socket;
	struct timeval now;
	struct timeval timeout_fd;

	init_omega(&dset);

	fd_udp_socket = fdd_init();
	FD_SET(fd_udp_socket, &dset);

    while (1) {
		memcpy(&active, &dset, sizeof(fd_set));

		gettimeofday(&now, NULL);

		fd_sched_run(&timeout_fd, &now);

		if( (selected = select(FD_SETSIZE, &active, NULL, NULL, &timeout_fd)) < 0 )
			terminate_omega(SIGINT);

		gettimeofday(&now, NULL);

		check_fd_socket(&active, &now);

		check_omega_socket(&active, &now);

		/* Check the omega registration FIFO */
		check_omega_fifo(&active, &dset);

		/* Check the cmd pipes of the registered processes */
		omega_local_check_pipes(&active, &dset, &now);
	}
	return 0;
}

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

#include <pthread.h>
#include <stdio.h>
#include "omega_msg.h"
#include <netinet/in.h>

#define OMEGA_FIFO_REG "/tmp/ddO_omega-reg" /* the fifo reg file */
#define OMEGA_FIFO_CMD "/tmp/ddO_omega-cmd-%u"
#define OMEGA_FIFO_QRY "/tmp/ddO_omega-qry-%u"
#define OMEGA_FIFO_INT "/tmp/ddO_omega-int-%u"

/* The udp port */
#define OMEGA_UDP_PORT 5555

/* A mutex used in the omegalib to access the registeredproc_list_head in mutual
 exclusion */
pthread_mutex_t registeredproc_list_mutex;

extern void terminate_omega(int sigint);
extern FILE *omega_log;

extern struct sockaddr_in omega_localaddr;


/* Omega Local module */
extern int omega_fifo_fd ;
extern struct list_head localregistered_proc_head;
extern void omega_local_init();
extern void check_omega_fifo(fd_set *active, fd_set *dset, struct timeval *now);
extern void omega_local_check_pipes(fd_set *active, fd_set *dset, struct timeval *now);

/* Returns the file descriptor of a local process pid */
extern int omega_group_exists_locally(unsigned int gid, int candidate);


/* Omega Fifo module */
extern int omega_fifo_init(void);
extern int omega_fifo_reinit(void);
extern void omega_fifo_cleanup(void);


/* Omega remote module */
extern int omega_udp_socket;
extern void check_omega_socket(fd_set *active, struct timeval *now);
extern void send_accusation(struct sockaddr_in *raddr, u_int gid, struct timeval *startTime);
extern void accusation_merge(char *msg, int msg_len, struct sockaddr_in *cliAddr, struct timeval *now);


extern struct list_head localLeader_head;
extern struct list_head localContenders_head;
extern struct list_head globalLeader_head;
extern struct list_head globalContenders_head;
extern int omega_algorithm_init();
extern int is_local_address(struct sockaddr_in *addr);
extern inline int add_proc_in_localContenders_set(struct sockaddr_in *addr, u_int pid, u_int gid);
extern inline int remove_proc_from_localContenders_set(struct sockaddr_in *addr, u_int pid, u_int gid);
extern inline int add_proc_in_globalContenders_set(struct sockaddr_in *addr, u_int pid, u_int gid);
extern inline int remove_proc_from_globalContenders_set(struct sockaddr_in *addr, u_int pid, u_int gid);
extern inline int updateLocalLeader(unsigned int gid, struct timeval *now);
extern inline int updateGlobalLeader(unsigned int gid, struct timeval *now);
extern int updateGlobalLeaders(struct timeval *now);
extern inline void doUponSuspected(struct sockaddr_in *addr, u_int pid, u_int gid, struct timeval *now);
extern inline void doUponReceivedAccusation(u_int gid, struct timeval *startTime, struct timeval *now);



/* Failure detector module */
extern int fdd_init();
extern void terminate_fdd();
extern void check_fd_socket(fd_set *active, struct timeval *now);
extern void fd_sched_run(struct timeval *timeout, struct timeval *now);

/* Failure detector local module */
extern int isalive(unsigned int pid);
extern int fdd_local_reg(unsigned int pid, struct timeval *now);
extern int fdd_local_unreg(unsigned int pid, struct timeval *now);
extern int do_join_group(unsigned int pid, unsigned int gid, struct timeval *now);
extern int do_join_group_invisible(unsigned int pid, unsigned int gid, int candidate, struct timeval *now);
extern int do_leave_group(unsigned int pid, unsigned int gid, struct timeval *now);
extern int do_monitor_group(unsigned int pid, unsigned int gid, unsigned int int_type, unsigned int TdU,
unsigned int TmU, unsigned int TmrL, struct timeval *now);
extern int do_stop_monitor_group(unsigned int pid, unsigned int gid, struct timeval *now);
extern int do_stop_sending_alives(unsigned int pid, unsigned int gid, struct timeval *now);
extern int do_restart_sending_alives(unsigned int pid, unsigned int gid, struct timeval *now);

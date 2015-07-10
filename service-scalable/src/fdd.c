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


/* fdd.c - main event loop */
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <signal.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include "fdd.h"
#include "misc.h"
#include "omega.h"
#include <pthread.h>
#include <errno.h>
#include <sys/resource.h>

/* exported stuff */
struct sockaddr_in fdd_local_addr ;

#ifdef LOG
FILE *flog ;
#endif

u_int host_sendint = 0 ;


/* local stuff */
static int fd_udp_socket; /* the socket file descriptor of the server */

extern struct list_head event_list_head ;
extern struct list_head local_procs_list_head ;

/* Added for Omega */
/* List containing for all the group g of all the local processes p if p
 is visible in g */
extern struct list_head visibility_list_head;

void terminate_fdd() {
  
  struct timeval now ;
  
  comm_cleanup();
  
  if(gettimeofday(&now, NULL) < 0)
    perror("gettimeofday") ;
  else {
#ifdef OUTPUT
    fprintf(stdout, "TERMINATED TS=%ld.%ld\n",
    now.tv_sec, now.tv_usec) ;
#endif
#ifdef LOG
    if(flog) {
      fprintf(flog, "TERMINATED TS=%ld.%ld\n",
      now.tv_sec, now.tv_usec) ;
      fclose(flog) ;
    }
#endif
  }
}

char* getNameFile() {
  struct timeval tv ;
  struct utsname uts ;
  char *fileName = NULL ;
  int len = 0 ;
  char *hostname, *p ;
  
  if (uname(&uts) < 0) {
    perror("fdd: uname");
    exit(EXIT_FAILURE) ;
  }
  p = strchr(uts.nodename, '.') ;
  if(p)
    len = p - uts.nodename ;
  else
    len = strlen(uts.nodename) ;
  
  hostname = malloc(sizeof(char)*(len+1)) ;
  memcpy(hostname, uts.nodename, len) ;
  hostname[len] = '\0' ;
  
  if( gettimeofday(&tv, NULL) < 0 ) {
    perror("gettimeofday:") ;
    exit(EXIT_FAILURE) ;
  }
  fileName = (char*)malloc(sizeof(char)*
    (4 + len + 1 + log10(tv.tv_sec) + 1
  + log10(tv.tv_usec) + 1 + 1) ) ;
  sprintf(fileName, "Fdd-%s-%ld.%ld", hostname, tv.tv_sec, tv.tv_usec) ;
  
  if(hostname)
    free(hostname) ;
  return fileName ;
}


/* check_fd_socket - reads a remote report from the UDP socket */
void check_fd_socket(fd_set *active, struct timeval *now) {
  char msg[SAFE_MSG_LEN] ;
  int msg_len ;
  struct sockaddr_in raddr;
  
  if(!FD_ISSET(fd_udp_socket, active))
    goto out;
  
  msg_len = comm_recv(msg, MAX_MSG_LEN, &raddr);
  if(msg_len < 0) {
    fprintf(stderr, "fdd: comm_recv() failed:%s\n", strerror(-msg_len));
    goto out ;
  }
  
  if(sockaddr_eq(&raddr, &fdd_local_addr)) { /* my message */
    goto out ;
  }
  
  switch (msg_type(msg)) {
    case MSG_REPORT:
    remote_merge(msg, msg_len, &raddr, now);
    break ;
    
    case MSG_HELLO:
      hello_multicast_merge(msg, msg_len, &raddr, now) ;
    break ;
    
    case MSG_INITIAL_ED:
    initial_ed_merge(msg, msg_len, &raddr, now) ;
    break ;
    
    default:
    fprintf(stderr, "fdd: bad message type on socket %d.\n",
    msg_type(msg));
    break;
  }
  out:
  return;
}


/* fdd_init - calls all initialization functions, returns the fd udp socket file
 descriptor */
extern int fdd_init() {
  struct sockaddr_in saddr ; /* broadcast socket address */
  struct utsname uts ;
  struct hostent *hst = NULL ;
  
#ifdef LOG
  char *fileName = NULL ;
#endif
  
  LIST_HEAD(mng_hosts) ;
  
  
  saddr.sin_addr.s_addr = htonl(DEFAULT_BADDR); /* XXX 64 bit clean ? */
  saddr.sin_port = htons(DEFAULT_PORT);
  
  /* retrive information about the current kernel and machine */
  if(uname(&uts) < 0) {
#ifdef OUTPUT
    perror("fdd: uname");
#endif
    goto fatal;
  }
  
#ifdef LOG
  fileName = getNameFile() ;
  if( (flog = fopen(fileName, "w")) == NULL ) {
    perror("Could not make the log file") ;
  }
  if(fileName)
    free(fileName) ;
#endif
  
  
  /* get the host like information about the current machine */
  hst = gethostbyname(uts.nodename);
  if (hst == NULL) {
    fprintf(stderr, "fdd: could not resolve %s\n", uts.nodename);
    goto fatal;
  }
  
  /* the network address of the current machine. */
  /* the FDD's socket address. */
  fdd_local_addr.sin_addr.s_addr = (*(u_long *)hst->h_addr);
  
#ifdef OUTPUT
  fprintf(stdout, "fdd: local name: %s\n", uts.nodename);
  fprintf(stdout, "fdd: local address: %u.%u.%u.%u\n",
  NIPQUAD(&fdd_local_addr));
  fprintf(stdout, "fdd: broadcast address: %u.%u.%u.%u\n", NIPQUAD(&saddr));
  fprintf(stdout, "fdd: port %u\n", ntohs(saddr.sin_port));
#endif
  
#ifdef LOG
  fprintf(flog, "fdd: local name: %s\n", uts.nodename);
  
  fprintf(flog, "fdd: local address: %u.%u.%u.%u\n",
  NIPQUAD(&fdd_local_addr));
  fprintf(flog, "fdd: broadcast address: %u.%u.%u.%u\n", NIPQUAD(&saddr));
  fprintf(flog, "fdd: port %u\n", ntohs(saddr.sin_port));
#endif
  
  /* open a socket for the broadcast communication. */
  if( (fd_udp_socket = comm_init(&saddr)) < 0 ) {
    fprintf(stderr, "fdd: comm_init() failed.\n");
    goto fatal;
  }
  
  
  /* local initialization stuff */
  local_init() ;
  
  return fd_udp_socket;
  
  fatal:
  exit(EXIT_FAILURE) ;
}



























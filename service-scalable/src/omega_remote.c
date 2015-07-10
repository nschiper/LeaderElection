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


/* omega_remote.c */

#include "omega_remote.h"
#include "msg.h"
#include "omega.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close */
#include <string.h>
#include "misc.h"


void send_accusation(struct sockaddr_in *raddr, u_int gid, struct timeval *startTime) {
  
  char msg[OMEGA_UDP_MSG_LEN], *ptr;
  int bytes;
  
#ifdef OMEGA_OUTPUT
  fprintf(stdout, "Sending accusation to: %u.%u.%u.%u for gid: %u startTime: %ld.%ld\n",
  NIPQUAD(raddr), gid, startTime->tv_sec, startTime->tv_usec);
#endif
#ifdef OMEGA_LOG
  fprintf(omega_log, "Sending accusation to: %u.%u.%u.%u for gid: %u startTime: %ld.%ld\n",
  NIPQUAD(raddr), gid, startTime->tv_sec, startTime->tv_usec);
#endif
  
  
  ptr = msg_omega_build_accusation(msg, gid, startTime);
  
  (*raddr).sin_family = AF_INET;
  (*raddr).sin_port = htons(OMEGA_UDP_PORT);
  
  bytes = sendto(omega_udp_socket, msg, ptr-msg, 0, (struct sockaddr *) raddr, sizeof(*raddr));
  
  if(bytes < 0)
    perror("omega: send_accusation(), cannot send data\n");
}


void accusation_merge(char *msg, int msg_len, struct sockaddr_in *cliAddr, struct timeval *now) {
  
  u_int gid;
  struct timeval startTime;
  
  msg_omega_parse_accusation(msg, &gid, &startTime);
  
  doUponReceivedAccusation(gid, &startTime, now);
  
#ifdef OMEGA_OUTPUT
  fprintf(stdout, "Received accusation from: %u.%u.%u.%u for gid: %u startTime: %ld.%ld\n",
  NIPQUAD(cliAddr), gid, startTime.tv_sec, startTime.tv_usec);
#endif
#ifdef OMEGA_LOG
  fprintf(omega_log, "Received accusation from: %u.%u.%u.%u for gid: %u startTime: %ld.%ld\n",
  NIPQUAD(cliAddr), gid, startTime.tv_sec, startTime.tv_usec);
#endif
}


/* check_omega_socket - reads a msg from the UDP socket */
void check_omega_socket(fd_set *active, struct timeval *now) {
  char msg[OMEGA_UDP_MSG_LEN] ;
  int msg_len, cliLen;
  struct sockaddr_in cliAddr;
  
  if(!FD_ISSET(omega_udp_socket, active))
    goto out;
  
  /* init msg */
  memset(msg, 0x0, OMEGA_UDP_MSG_LEN);
  
  cliLen = sizeof(cliAddr);
  
  msg_len = recvfrom(omega_udp_socket, msg, OMEGA_UDP_MSG_LEN, 0, (struct sockaddr *) &cliAddr, &cliLen);
  if(msg_len < 0) {
    perror("omega: cannot receive data from udp socket\n");
    goto out;
  }
  
  switch (msg_type(msg)) {
    case MSG_OMEGA_ACCUSATION:
    accusation_merge(msg, msg_len, &cliAddr, now);
    break ;
    
    default:
      fprintf(stderr, "omega: bad message type on udp socket %d.\n", msg_type(msg));
    break;
  }
  out:
  return;
}

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


/* comm.c - UDP communication primitives */
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "fdd.h"

static struct sockaddr_in saddr ;
static struct sockaddr_in maddr ; /* multicast address */
static int fd_udp_socket ;

/* comm_init - initilization. creates a broadcast UDP socket */
extern int comm_init(struct sockaddr_in *addr) {
  struct sockaddr_in laddr ;
  int retval = 0 ;
  struct ip_mreq mreq ;
  
  saddr.sin_family = PF_INET ;
  saddr.sin_addr.s_addr = addr->sin_addr.s_addr ;
  saddr.sin_port = addr->sin_port ;
  
  maddr.sin_family = PF_INET ;
  
#if defined(sun)
  maddr.sin_addr.s_addr = inet_addr(HELLO_GROUP) ;
  if( maddr.sin_addr.s_addr == -1 )
    retval = -1 ;
#else
  retval = inet_aton(HELLO_GROUP, &maddr.sin_addr) ;
#endif
  if(retval < 0)
    goto error ;
  
  maddr.sin_port = htons(DEFAULT_PORT) ;
  
  fd_udp_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) ;
  if (fd_udp_socket == -1) {
    retval = -errno ;
    goto error ;
  }
  
  bzero(&laddr, sizeof(laddr)) ;
  laddr.sin_family = AF_INET ;
  laddr.sin_port = saddr.sin_port ;
  laddr.sin_addr.s_addr = htonl(INADDR_ANY) ;
  
  if(bind(fd_udp_socket, (struct sockaddr *)&laddr, sizeof(laddr)) == -1) {
    retval = -errno ;
    goto error_close ;
  }
  
#if defined(sun)
  mreq.imr_multiaddr.s_addr = inet_addr(HELLO_GROUP) ;
  if(mreq.imr_multiaddr.s_addr == -1)
    retval = -1 ;
#else
  retval = inet_aton(HELLO_GROUP, &mreq.imr_multiaddr) ;
#endif
  if(retval < 0)
    goto error_close ;
  
  mreq.imr_interface.s_addr = htonl(INADDR_ANY) ;
  
  if(setsockopt(fd_udp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
    retval = -errno ;
    goto error_close ;
  }
  
  if( retval < 0 )
    goto error_close ;
  
  return fd_udp_socket;
  
  error_close:
  close(fd_udp_socket);
  error:
  return retval;
}

extern void comm_cleanup(void) {
  close(fd_udp_socket);
}

extern int comm_recv(char *buf, int len, struct sockaddr_in *raddr) {
  int rsize = sizeof(*raddr) ;
  int bytes = 0 ;
  
  bytes = recvfrom(fd_udp_socket, buf, len, 0, (struct sockaddr *)raddr, &rsize) ;
  if(bytes == -1)
    return -errno ;
  return bytes ;
}

extern int comm_bcast(char *buf, int len) {
  return comm_send(buf, len, &saddr);
}

extern int comm_send(char *buf, int len, struct sockaddr_in *raddr) {
  int bytes = 0 ;
  
  (*raddr).sin_port = htons(DEFAULT_PORT);
  bytes = sendto(fd_udp_socket, buf, len, 0,
  (struct sockaddr *)raddr, sizeof(*raddr));
  
  if(bytes == -1) {
    perror("comm_send = -1") ;
    return -errno;
  }
  if(bytes < len) {
    perror("comm_send < len") ;
    return -EMSGSIZE;
  }
  return bytes;
}

extern int comm_hello_multicast(char *buf, int len) {
  return comm_send(buf, len, &maddr) ;
}

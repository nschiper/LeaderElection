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


/* fdd_msg.h */
/* FIXME: this file really wants to be automatically generated */
#ifndef _MSG_H
#define _MSG_H

#define __USE_GNU

#include <string.h>
#include <sys/time.h>
#include "fdd_types.h"
#include "msg.h"
#include "variables_exchange.h"


#define SAFE_MSG_LEN (MAX_MSG_LEN + 24)
#define HELLO_LEN 16

#define MSG_REPORT		 0
#define MSG_RESULT		 1
#define MSG_REGISTER		 2

#define MSG_MONITOR_PROC	 3
#define MSG_STOP_MONITOR_PROC    4

#define MSG_QUERY_PROC		 5
#define MSG_QUERY_GROUP          6
#define MSG_QUERY_ALL   	 7

#define MSG_INTERRUPT_ON_PROC	 8
#define MSG_INTERRUPT_ON_GROUP   9
#define MSG_NOTIFY		10
#define MSG_VALIDITY            11
#define MSG_HELLO               12

#define MSG_JOIN_GROUP          13
#define MSG_LEAVE_GROUP         14
#define MSG_MONITOR_GROUP       15
#define MSG_STOP_MONITOR_GROUP  16

#define MSG_INITIAL_ED          17

/* Added for Omega */
/* Added to be able to decide locally if we want to send alive messages
or not. It is mandatory to implement a communication-efficient Omega.
The third message type is when we want to join a group but be invisible
 from the beginning. */
#define MSG_STOP_SENDING_ALIVES    18
#define MSG_RESTART_SENDING_ALIVES 19
#define MSG_JOIN_GROUP_INVISIBLE   20

/* max size of MSG_REGISTER, MSG_MONITOR_ALL, MSG_MONITOR_PROC,
 MSG_INTERRUPT_ON, MSG_NOTIFY, MSG_RESULT, ... */
/* FIXME: in most cases we know the exact size */
#define FIFO_MSG_LEN (8*4)

/*
 * Registration message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_REGISTER)
 *	4    bytes  pid		(process ID)
 */

static inline char *msg_build_reg(char *msg, u_int pid)
{
  char *ptr = msg;
  put32(ptr, (unsigned int)MSG_REGISTER);  ptr += 4;
  put32(ptr, (unsigned int)pid); ptr += 4;
  return ptr;
}

static inline char *msg_parse_reg(char *msg, u_int *pid) {
  char *ptr = msg + 4;
  *pid = get32(ptr); ptr += 4;
  return ptr;
}

/*
 * JOIN_GROUP message format (all fields are network byte order) :
 *      4    bytes  type        ( message type - MSG_JOIN_GROUP)
 *      4    bytes  gid         ( group ID )
 */
static inline char *msg_build_join_group(char *msg, u_int gid) {
  char *ptr = msg ;
  put32(ptr, (unsigned int)MSG_JOIN_GROUP); ptr += 4 ;
  put32(ptr, (unsigned int)gid); ptr += 4 ;
  return ptr;
}
static inline char *msg_parse_join_group(char *msg, u_int *gid) {
  char *ptr = msg + 4 ;
  *gid = get32(ptr); ptr += 4;
  return ptr;
}

/*
 * MONITOR_GROUP message format (all fields are network byte order) :
 *      4    bytes  type        ( message type - MSG_MONITOR_GROUP)
 *      4    bytes  gid         ( group ID )
 *      4    bytes  TdU
 *      4    bytes  TmU
 *      4    bytes  TmrL
 */

static inline char *msg_build_monitor_group(char *msg, u_int gid,
  u_int int_type, unsigned int TdU,
  unsigned int TmU,
  unsigned int TmrL) {
  char *ptr = msg ;
  put32(ptr, (unsigned int)MSG_MONITOR_GROUP); ptr += 4 ;
  put32(ptr, (unsigned int)gid); ptr += 4 ;
  put32(ptr, (unsigned int)int_type) ; ptr += 4 ;
  put32(ptr, (unsigned int)TdU); ptr += 4 ;
  put32(ptr, (unsigned int)TmU); ptr += 4 ;
  put32(ptr, (unsigned int)TmrL); ptr += 4 ;
  
  return ptr ;
}

static inline char *msg_parse_monitor_group(char *msg, u_int *gid,
  u_int *int_type,
  unsigned int *TdU,
  unsigned int *TmU,
  unsigned int *TmrL) {
  char *ptr = msg + 4 ;
  *gid = get32(ptr); ptr += 4 ;
  *int_type = get32(ptr) ; ptr += 4 ;
  *TdU = get32(ptr); ptr += 4 ;
  *TmU = get32(ptr); ptr += 4 ;
  *TmrL = get32(ptr); ptr += 4 ;
  return ptr;
}

/*
 * STOP_MONITOR_GROUP message format (all fields are network byte order) :
 *      4    bytes  type        ( message type - MSG_STOP_MONITOR_GROUP)
 *      4    bytes  gid         ( group ID )
 */

static inline char *msg_build_stop_monitor_group(char *msg, u_int gid) {
  char *ptr = msg ;
  put32(ptr, (unsigned int)MSG_STOP_MONITOR_GROUP); ptr += 4 ;
  put32(ptr, (unsigned int)gid); ptr += 4 ;
  return ptr;
}
static inline char *msg_parse_stop_monitor_group(char *msg, u_int *gid) {
  char *ptr = msg + 4 ;
  *gid = get32(ptr); ptr += 4;
  return ptr;
}


/* added for Omega */
/*
 * MSG_STOP_SENDING_ALIVES message format (all fields are network byte order) :
 *      4    bytes  type        ( message type - MSG_LEAVE_GROUP)
 *      4    bytes  gid         ( group ID )
 */
static inline char *msg_build_stop_sending_alives(char *msg, u_int gid) {
  char *ptr = msg ;
  put32(ptr, (unsigned int)MSG_STOP_SENDING_ALIVES); ptr += 4 ;
  put32(ptr, (unsigned int)gid); ptr += 4 ;
  return ptr;
}
static inline char *msg_parse_stop_sending_alives(char *msg, u_int *gid) {
  char *ptr = msg + 4 ;
  *gid = get32(ptr); ptr += 4;
  return ptr;
}

/* added for Omega */
/*
 * MSG_RESTART_SENDING_ALIVES message format (all fields are network byte order) :
 *      4    bytes  type        ( message type - MSG_LEAVE_GROUP)
 *      4    bytes  gid         ( group ID )
 */
static inline char *msg_build_restart_sending_alives(char *msg, u_int gid) {
  char *ptr = msg ;
  put32(ptr, (unsigned int)MSG_RESTART_SENDING_ALIVES); ptr += 4 ;
  put32(ptr, (unsigned int)gid); ptr += 4 ;
  return ptr;
}
static inline char *msg_parse_restart_sending_alives(char *msg, u_int *gid) {
  char *ptr = msg + 4 ;
  *gid = get32(ptr); ptr += 4;
  return ptr;
}


/* added for Omega */
/*
 * MSG_JOIN_GROUP_INVISIBLE message format (all fields are network byte order) :
 *      4    bytes  type        ( message type - MSG_LEAVE_GROUP)
 *      4    bytes  gid         ( group ID )
 */
static inline char *msg_build_join_group_invisible(char *msg, u_int gid) {
  char *ptr = msg ;
  put32(ptr, (unsigned int)MSG_JOIN_GROUP_INVISIBLE); ptr += 4 ;
  put32(ptr, (unsigned int)gid); ptr += 4 ;
  return ptr;
}
static inline char *msg_parse_join_group_invisible(char *msg, u_int *gid) {
  char *ptr = msg + 4 ;
  *gid = get32(ptr); ptr += 4;
  return ptr;
}



/*
 * LEAVE_GROUP message format (all fields are network byte order) :
 *      4    bytes  type        ( message type - MSG_LEAVE_GROUP)
 *      4    bytes  gid         ( group ID )
 */
static inline char *msg_build_leave_group(char *msg, u_int gid) {
  char *ptr = msg ;
  put32(ptr, (unsigned int)MSG_LEAVE_GROUP); ptr += 4 ;
  put32(ptr, (unsigned int)gid); ptr += 4 ;
  return ptr;
}
static inline char *msg_parse_leave_group(char *msg, u_int *gid) {
  char *ptr = msg + 4 ;
  *gid = get32(ptr); ptr += 4;
  return ptr;
}

/*
 * Monitor Process message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_MONITOR_PROC)
 *	4    bytes  pid		(process to be monitored)
 *	4    bytes  host	(IP address of host)
 *      4    bytes  int_type    (type of interruption desired)
 *	4    bytes  TdU		(upper bound on detection time)
 *	4    bytes  TmU		(upper bound on mistake duration)
 *	4    bytes  TmrL	(lower bound on mistake recurrence time)
 */

static inline char *msg_build_monproc(char *msg,
  u_int pid, struct sockaddr_in *addr,
  u_int int_type,
u_int TdU, u_int TmU, u_int TmrL)
{
  char *ptr = msg;
  
  put32(ptr, (unsigned int)MSG_MONITOR_PROC);  ptr += 4;
  put32(ptr, (unsigned int)pid); ptr += 4;
  put32(ptr, (unsigned int)ntohl(addr->sin_addr.s_addr)); ptr += 4;
  put32(ptr, (unsigned int)int_type) ; ptr += 4 ;
  put32(ptr, (unsigned int)TdU); ptr += 4;
  put32(ptr, (unsigned int)TmU); ptr += 4;
  put32(ptr, (unsigned int)TmrL); ptr += 4;
  
  return ptr;
}

static inline char *msg_parse_monproc(char *msg, u_int *pid,
  struct sockaddr_in *addr,
  u_int *int_type,
u_int *TdU, u_int *TmU, u_int *TmrL)
{
  char *ptr = msg + 4;
  
  *pid = get32(ptr); ptr += 4;
  addr->sin_addr.s_addr = htonl(get32(ptr)); ptr += 4;
  *int_type = get32(ptr) ; ptr += 4 ;
  *TdU = get32(ptr); ptr += 4;
  *TmU = get32(ptr); ptr += 4;
  *TmrL = get32(ptr); ptr += 4;
  
  return ptr;
}


/*
 * Stop Monitor Process message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_STOP_MONITOR_PROC)
 *	4    bytes  pid		(process to be monitored)
 *	4    bytes  host	(IP address of host)
 */

static inline char *msg_build_stop_monproc(char *msg,
u_int pid, struct sockaddr_in *addr)
{
  char *ptr = msg;
  
  put32(ptr, (unsigned int)MSG_STOP_MONITOR_PROC);  ptr += 4;
  put32(ptr, (unsigned int)pid); ptr += 4;
  put32(ptr, (unsigned int)ntohl(addr->sin_addr.s_addr)); ptr += 4;
  return ptr;
}

static inline char *msg_parse_stop_monproc(char *msg,
u_int *pid, struct sockaddr_in *addr)
{
  char *ptr = msg + 4;
  
  *pid = get32(ptr); ptr += 4;
  addr->sin_addr.s_addr = htonl(get32(ptr)); ptr += 4;
  return ptr;
}


/*
 * Query All message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_QUERY_ALL)
 *
 * Extra reply message format (pcount passed in extra filed in MSG_RESULT)
 *	pcount times:
 *		4    bytes  pid
 *              4    bytes  gid
 *              4    bytes  group_available
 *              4    bytes  trust
 *		4    bytes  host	(IP address)
 */

#define FDD_QUERY_PROC_LEN 24

static inline char *msg_build_qryall_head(char *msg)
{
  char *ptr = msg ;
  
  put32(ptr, (unsigned int)MSG_QUERY_ALL) ;  ptr += 4 ;
  
  return ptr ;
}

static inline char *msg_build_qryall_proc(char *msg, u_int pid,
  unsigned int gid,
  unsigned int group_available,
  unsigned int trust,
struct sockaddr_in *addr)
{
  char *ptr = msg;
  
  put32(ptr, (unsigned int)pid); ptr += 4;
  put32(ptr, (unsigned int)gid); ptr += 4;
  put32(ptr, (unsigned int)group_available); ptr += 4 ;
  put32(ptr, (unsigned int)trust); ptr += 4 ;
  put32(ptr, (unsigned int)ntohl(addr->sin_addr.s_addr)); ptr += 4;
  
  return ptr;
}

static inline char *msg_parse_qryall_proc(char *msg, u_int *pid,
  unsigned int *gid,
  unsigned int *group_available,
  unsigned int *trust,
struct sockaddr_in *addr)
{
  char *ptr = msg;
  
  *pid = get32(ptr); ptr += 4;
  *gid = get32(ptr); ptr += 4;
  *group_available = get32(ptr); ptr += 4;
  *trust = get32(ptr); ptr += 4 ;
  addr->sin_addr.s_addr = htonl(get32(ptr)); ptr += 4;
  
  return ptr;
}


/*
 * Query Group message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_QUERY_ALL)
 *      4    bytes  gid
 *
 * Extra reply message format (pcount passed in extra filed in MSG_RESULT)
 *	pcount times:
 *		4    bytes  pid
 *              4    bytes  gid
 *              4    bytes  group_available
 *              4    bytes  trust
 *		4    bytes  host	(IP address)
 */

static inline char *msg_build_qrygroup_head(char *msg, unsigned int gid)
{
  char *ptr = msg ;
  
  put32(ptr, (unsigned int)MSG_QUERY_GROUP) ;  ptr += 4 ;
  put32(ptr, (unsigned int)gid); ptr += 4 ;
  
  
  return ptr ;
}

static inline char *msg_parse_qrygroup_head(char *msg, unsigned int *gid) {
  char *ptr = msg + 4 ;
  
  *gid = get32(ptr) ; ptr += 4 ;
  
  return ptr ;
}

static inline char *msg_build_qrygroup_proc(char *msg, u_int pid,
  unsigned int gid,
  unsigned int group_available,
  unsigned int trust,
struct sockaddr_in *addr)
{
  char *ptr = msg;
  
  put32(ptr, (unsigned int)pid); ptr += 4;
  put32(ptr, (unsigned int)gid); ptr += 4;
  put32(ptr, (unsigned int)group_available); ptr += 4 ;
  put32(ptr, (unsigned int)trust); ptr += 4 ;
  put32(ptr, (unsigned int)ntohl(addr->sin_addr.s_addr)); ptr += 4;
  
  return ptr;
}

static inline char *msg_parse_qrygroup_proc(char *msg, u_int *pid,
  unsigned int *gid,
  unsigned int *group_available,
  unsigned int *trust,
struct sockaddr_in *addr)
{
  char *ptr = msg;
  
  *pid = get32(ptr); ptr += 4;
  *gid = get32(ptr); ptr += 4;
  *group_available = get32(ptr); ptr += 4;
  *trust = get32(ptr); ptr += 4 ;
  addr->sin_addr.s_addr = htonl(get32(ptr)); ptr += 4;
  
  return ptr;
}


/*
 * Query Proc message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_QUERY_PROC)
 *	4    bytes  pid
 *      4    bytes  gid
 *      4    bytes  group_available
 *	4    bytes  host	(IP address)
 *
 * Result = 0 -> suspect
 * Result = 1 -> trust
 */

static inline char *msg_build_qryproc(char *msg, u_int pid,
  unsigned int gid,
  unsigned int group_available,
struct sockaddr_in *addr)
{
  char *ptr = msg;
  
  put32(ptr, (unsigned int)MSG_QUERY_PROC);  ptr += 4;
  put32(ptr, (unsigned int)pid); ptr += 4;
  put32(ptr, (unsigned int)gid); ptr += 4 ;
  put32(ptr, (unsigned int)group_available); ptr += 4;
  put32(ptr, (unsigned int)ntohl(addr->sin_addr.s_addr)); ptr += 4;
  
  return ptr;
}

static inline char *msg_parse_qryproc(char *msg, u_int *pid,
  unsigned int *gid,
  unsigned int *group_available,
struct sockaddr_in *addr)
{
  char *ptr = msg + 4;
  
  *pid = get32(ptr); ptr += 4;
  *gid = get32(ptr); ptr += 4;
  *group_available = get32(ptr); ptr += 4;
  addr->sin_addr.s_addr = htonl(get32(ptr)); ptr += 4;
  
  return ptr;
}

/*
 * Interrupt On Process message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_INTERRUPT_ON_PROC)
 *      4    bytes  pid         (monitored proc's pid)
 *      4    bytes  host        (IP address of proc)
 *	4    bytes  int_type	(INTERRUPT_NONE, INTERRUPT_ANY_CHANGE)
 */

static inline char *msg_build_int_proc(char *msg, u_int pid,
  struct sockaddr_in *addr,
  int int_type) {
  char *ptr = msg;
  
  put32(ptr, (unsigned int)MSG_INTERRUPT_ON_PROC);  ptr += 4;
  put32(ptr, (unsigned int)pid) ; ptr += 4 ;
  put32(ptr, (unsigned int)ntohl(addr->sin_addr.s_addr)); ptr += 4;
  put32(ptr, (unsigned int)int_type); ptr += 4;
  
  return ptr;
}

static inline char *msg_parse_int_proc(char *msg, u_int *pid,
  struct sockaddr_in *addr,
  u_int *int_type) {
  char *ptr = msg + 4;
  
  *pid = get32(ptr); ptr += 4;
  addr->sin_addr.s_addr = htonl(get32(ptr)); ptr += 4;
  *int_type = get32(ptr) ; ptr += 4 ;
  
  return ptr;
}

/*
 * Interrupt On Group message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_INTERRUPT_ON_GROUP)
 *      4    bytes  gid         (monitored group's gid)
 *	4    bytes  int_type	(INTERRUPT_NONE, INTERRUPT_ANY_CHANGE)
 */

static inline char *msg_build_int_group(char *msg, u_int gid,
  int int_type) {
  char *ptr = msg;
  
  put32(ptr, (unsigned int)MSG_INTERRUPT_ON_GROUP);  ptr += 4;
  put32(ptr, (unsigned int)gid) ; ptr += 4 ;
  put32(ptr, (unsigned int)int_type); ptr += 4;
  
  return ptr;
}

static inline char *msg_parse_int_group(char *msg, u_int *gid,
  u_int *int_type) {
  char *ptr = msg + 4;
  
  *gid = get32(ptr); ptr += 4;
  *int_type = get32(ptr) ; ptr += 4 ;
  
  return ptr;
}

/*
 * Notification message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_NOTIFY)
 *	4    bytes  trust	(1 - trusted, 0 - suspected, 2 - suspected because of the TO)
 *	4    bytes  pid
 *      4    bytes  gid
 *      4    bytes  group_available (1-group monitoring, gid valid; 0-pt. to pt. gid no valid)
 *	4    bytes  host	(IP address of host)
 *	8    bytes  timestamp
 */

#define TRUST_NOTIF       0
#define SUSPECT_NOTIF     1
#define CRASHED_NOTIF     2

static inline char *msg_build_not(char *msg,
  unsigned int trust,
  unsigned int pid,
  unsigned int gid,
  unsigned int group_available,
  struct sockaddr_in *addr,
struct timeval *tv)
{
  char *ptr = msg;
  
  put32(ptr, (unsigned int)MSG_NOTIFY);  ptr += 4;
  put32(ptr, (unsigned int)trust); ptr += 4;
  put32(ptr, (unsigned int)pid); ptr += 4;
  put32(ptr, (unsigned int)gid); ptr += 4 ;
  put32(ptr, (unsigned int)group_available); ptr += 4;
  put32(ptr, (unsigned int)ntohl(addr->sin_addr.s_addr)); ptr += 4;
  put32(ptr, (unsigned int)tv->tv_sec); ptr += 4;
  put32(ptr, (unsigned int)tv->tv_usec); ptr += 4;
  
  return ptr;
}

static inline char *msg_parse_not(char *msg,
  unsigned int *trust,
  unsigned int *pid,
  unsigned int *gid,
  unsigned int *group_available,
  struct sockaddr_in *addr,
struct timeval *tv)
{
  char *ptr = msg + 4;
  
  *trust = get32(ptr); ptr += 4;
  *pid = get32(ptr); ptr += 4;
  *gid = get32(ptr); ptr += 4 ;
  *group_available = get32(ptr); ptr += 4 ;
  addr->sin_addr.s_addr = htonl(get32(ptr)); ptr += 4;
  tv->tv_sec = get32(ptr); ptr += 4;
  tv->tv_usec = get32(ptr); ptr += 4;
  
  return ptr;
}

/*
 * Result message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_RESULT)
 *	4    bytes  result
 *	4    bytes  extra	(number of additional response messages)
 *	8    bytes  timestamp	(when the request was processed)
 */

static inline char *msg_build_res(char *msg, int result, int extra,
struct timeval *tv)
{
  char *ptr = msg;
  
  put32(ptr, (unsigned int)MSG_RESULT);  ptr += 4;
  put32(ptr, (unsigned int)result); ptr += 4;
  put32(ptr, (unsigned int)extra); ptr += 4;
  put32(ptr, (unsigned int)tv->tv_sec); ptr += 4;
  put32(ptr, (unsigned int)tv->tv_usec); ptr += 4;
  
  return ptr;
}

static inline char *msg_parse_res(char *msg, int *result, int *extra,
struct timeval *tv)
{
  char *ptr = msg + 4;
  
  *result = get32(ptr); ptr += 4;
  *extra = get32(ptr); ptr += 4;
  tv->tv_sec = get32(ptr); ptr += 4;
  tv->tv_usec = get32(ptr); ptr += 4;
  
  return ptr;
}

/**************************************************************************
 * IMPORTANT:::                                                           *
 *     local_* and remote_* stuff are with respect to the sender machine  *
 **************************************************************************
 *  local_server_* stuff is related to the processes from the local host that are monitored
 *                 by remote host
 *  remote_server_* stuff is related to the processes from the remote host that are monitored
 *                 by the local host
 *
 *
 * Report message format:
 * head:
 *	4    bytes  type	           (MSG_REPORT)
 *      8    bytes  timestamp
 *	4    bytes  seq		            (sequence number)
 *	8    bytes  epoch	            (epoch timestamp of local host start)
 *      8    bytes  remote_epoch            (epoch timestamp of the remote host)
 * ------- the local host ------------
 *	4    bytes  local_servers_list_seq  (sequence number of the list of server processes)
 *      4    bytes  local_groups_list_seq  (seq. nb. of the list of remote groups)
 *	4    bytes  local_servers_list_len  (length of the list of server  processes)
 *      4    bytes  local_servers_proc_count(nb. of local servers that doesn't belong to any group)
 *      4    bytes  local_groups_count      (number of local groups that are sent)
 *	4    bytes  local_sendint           (sending interval of the local host)
 *  4    bytes localvars_count          (nb of localvars of processes)
 * ------- the remote host -----------
 *      4    bytes  local_clients_list_seq  (sequence number of the list of client processes)
 *	4    bytes  remote_servers_list_len (length of the list of client processes)
 *      4    bytes  remote_servers_proc_count(number of remote servers  that doesn't belong to any group)
 *	4    bytes  remote_needed_sendint  (sendint needed by the local host from the remote)
 *  8    bytes  remote_largest_group_ts   (The largest remote group timestamp received)
 *
 *
 * local servers processes list;
 *     local_servers_proc_count times
 *          4    bytes   pid
 *
 * local groups list:
 *     local_groups_count times
 *          4    bytes   gid
 *          8    bytes   group_ts     (time of first process to join group gid or time
of invisibile->visible transition for group gid)
 *          4    bytes   eta_rcvd     (tells if the eta has been received for this group)
 *          4    bytes   proc_count    nb of processes member of this particular group on this machine
 *           proc_count times
 *               4    bytes    pid            (process ID)
 *
 * remote servers processes list:
 *          4    bytes    pid            (process ID)
 *
 *
 * localvars list;    (list of local variables)
 *    localvars_count times
 *          4 bytes      gid
 *          4 bytes      accusationTime.tv_sec
 *          4 bytes      accusationTime.tv_usec
 *          4 bytes      bestAmongActives_pid
 *          4 bytes      bestAmongActives_addr
 *          4 bytes      bestAmongActives_accTime.tv_sec
 *          4 bytes      bestAmongActives_accTime.tv_usec
 */

/* The function skips the header of the report  message */
static inline char *msg_skip_rep_head(char *msg) {
  return msg + 21*4;
}


static inline char *msg_build_rep_head(char *msg, struct timeval *tv,
  u_int seq, struct timeval *local_epoch,
  struct timeval *remote_epoch,
  
  u_int local_servers_list_seq,
  u_int local_groups_list_seq,
  u_int local_servers_list_len,
  u_int local_servers_proc_count,
  u_int local_groups_count,
  u_int local_sendint,
  u_int localvars_count,
  
  u_int local_clients_list_seq,
  u_int remote_servers_list_len,
  u_int remote_servers_proc_count,
  u_int remote_needed_sendint,
  struct timeval *remote_largest_group_ts) {
  char *ptr = msg;
  
  put32(ptr, (unsigned int)MSG_REPORT);  ptr += 4 ;
  put32(ptr, (unsigned int)tv->tv_sec); ptr += 4 ;
  put32(ptr, (unsigned int)tv->tv_usec); ptr += 4 ;
  put32(ptr, (unsigned int)seq); ptr += 4 ;
  put32(ptr, (unsigned int)local_epoch->tv_sec); ptr += 4 ;
  put32(ptr, (unsigned int)local_epoch->tv_usec); ptr += 4 ;
  put32(ptr, (unsigned int)remote_epoch->tv_sec); ptr += 4 ;
  put32(ptr, (unsigned int)remote_epoch->tv_usec); ptr += 4 ;
  
  put32(ptr, (unsigned int)local_servers_list_seq); ptr += 4 ;
  put32(ptr, (unsigned int)local_groups_list_seq); ptr += 4 ;
  put32(ptr, (unsigned int)local_servers_list_len); ptr += 4 ;
  put32(ptr, (unsigned int)local_servers_proc_count); ptr += 4 ;
  put32(ptr, (unsigned int)local_groups_count); ptr += 4 ;
  put32(ptr, (unsigned int)local_sendint); ptr += 4 ;
  put32(ptr, (unsigned int)localvars_count); ptr+=4;
  
  put32(ptr, (unsigned int)local_clients_list_seq); ptr += 4 ;
  put32(ptr, (unsigned int)remote_servers_list_len); ptr += 4 ;
  put32(ptr, (unsigned int)remote_servers_proc_count); ptr += 4 ;
  put32(ptr, (unsigned int)remote_needed_sendint); ptr += 4 ;
  put32(ptr, (unsigned int)remote_largest_group_ts->tv_sec); ptr += 4 ;
  put32(ptr, (unsigned int)remote_largest_group_ts->tv_usec); ptr += 4 ;
  
  return ptr;
}


static inline
char *msg_parse_rep_head(char *msg, struct timeval *tv,
  u_int *seq, struct timeval *remote_epoch,
  struct timeval *local_epoch,
  
  u_int *remote_servers_list_seq,
  u_int *remote_groups_list_seq,
  u_int *remote_servers_list_len,
  u_int *remote_servers_proc_count,
  u_int *remote_groups_count,
  u_int *remote_sendint,
  u_int *remotevars_count,
  
  u_int *remote_clients_list_seq,
  u_int *local_servers_list_len,
  u_int *local_servers_proc_count,
  u_int *local_needed_sendint,
  struct timeval *local_largest_group_ts) {
  
  char *ptr = msg + 4;
  
  tv->tv_sec = get32(ptr); ptr += 4;
  tv->tv_usec = get32(ptr); ptr += 4;
  *seq = get32(ptr); ptr += 4;
  remote_epoch->tv_sec = get32(ptr); ptr += 4;
  remote_epoch->tv_usec = get32(ptr); ptr += 4;
  local_epoch->tv_sec = get32(ptr); ptr += 4;
  local_epoch->tv_usec = get32(ptr); ptr += 4;
  
  *remote_servers_list_seq = get32(ptr); ptr += 4;
  *remote_groups_list_seq = get32(ptr); ptr += 4 ;
  *remote_servers_list_len = get32(ptr); ptr += 4;
  *remote_servers_proc_count = get32(ptr); ptr += 4;
  *remote_groups_count = get32(ptr); ptr += 4;
  *remote_sendint = get32(ptr); ptr += 4;
  *remotevars_count = get32(ptr); ptr += 4;
  
  *remote_clients_list_seq = get32(ptr); ptr += 4;
  *local_servers_list_len = get32(ptr); ptr += 4;
  *local_servers_proc_count = get32(ptr); ptr += 4;
  *local_needed_sendint = get32(ptr); ptr += 4;
  local_largest_group_ts->tv_sec = get32(ptr); ptr += 4;
  local_largest_group_ts->tv_usec = get32(ptr); ptr += 4;
  
  return ptr;
}


static inline char *msg_skip_rep_ghead(char *msg)
{
  return msg + 5*4;
}
static inline char *msg_build_rep_ghead(char *msg, u_int gid, struct timeval *group_ts,
u_int eta_rcvd, int procs_count)
{
  char *ptr = msg;
  
  put32(ptr, (unsigned int)gid); ptr += 4;
  put32(ptr, (unsigned int)group_ts->tv_sec); ptr += 4;
  put32(ptr, (unsigned int)group_ts->tv_usec); ptr += 4;
  put32(ptr, (unsigned int)eta_rcvd); ptr += 4;
  put32(ptr, (unsigned int)procs_count); ptr += 4;
  
  return ptr;
}
static inline char *msg_parse_rep_ghead(char *msg, u_int *gid, struct timeval *group_ts,
u_int *eta_rcvd, int *procs_count)
{
  char *ptr = msg;
  
  *gid = get32(ptr); ptr += 4;
  group_ts->tv_sec = get32(ptr); ptr += 4;
  group_ts->tv_usec = get32(ptr); ptr += 4;
  *eta_rcvd = get32(ptr); ptr += 4;
  *procs_count = get32(ptr); ptr += 4;
  
  return ptr;
}

static inline char *msg_build_rep_pid(char *msg, u_int pid) {
  char *ptr = msg;
  
  put32(ptr, (unsigned int)pid); ptr += 4;
  
  return ptr;
}

static inline char *msg_parse_rep_pid(char *msg, u_int *pid)
{
  char *ptr = msg;
  
  *pid = get32(ptr); ptr += 4;
  
  return ptr;
}


static inline char *msg_build_rep_localvars(char *msg, u_int gid, struct timeval *accusationTime,
  struct bestAmongActives_struct *bestAmongActives,
struct timeval *bestAmongActives_accTime)
{
  char *ptr = msg;
  
  put32(ptr, (unsigned int)gid); ptr += 4;
  put32(ptr, (unsigned int)accusationTime->tv_sec); ptr+=4;
  put32(ptr, (unsigned int)accusationTime->tv_usec); ptr+=4;
  put32(ptr, (unsigned int)bestAmongActives->pid); ptr+=4;
  put32(ptr, (unsigned int)ntohl(bestAmongActives->addr.sin_addr.s_addr)); ptr += 4;
  put32(ptr, (unsigned int)bestAmongActives_accTime->tv_sec); ptr+=4;
  put32(ptr, (unsigned int)bestAmongActives_accTime->tv_usec); ptr+=4;
  return ptr;
}
static inline char *msg_parse_rep_localvars(char *msg, u_int *gid, struct timeval *accusationTime,
  struct bestAmongActives_struct *bestAmongActives,
struct timeval *bestAmongActives_accTime)
{
  char *ptr = msg;
  
  *gid = get32(ptr); ptr += 4;
  accusationTime->tv_sec = get32(ptr); ptr += 4;
  accusationTime->tv_usec = get32(ptr); ptr += 4;
  bestAmongActives->pid = get32(ptr); ptr += 4;
  bestAmongActives->addr.sin_addr.s_addr = htonl(get32(ptr)); ptr += 4;
  bestAmongActives_accTime->tv_sec = get32(ptr); ptr += 4;
  bestAmongActives_accTime->tv_usec = get32(ptr); ptr += 4;
  return ptr;
}



/*
 * Hello message format
 * head
 *       4    bytes     type        (message type - MSG_HELLO)
 *       4    bytes     hello_seq
 *       4    bytes     local_groups_list_seq
 *       8    bytes     epoch timestamp
 *       4    bytes     groups_count
 * process list
 *    groups_count times:
 *            4    bytes    gid
 */


static inline char *msg_skip_hello_head(char *msg) {
  return msg + 6*4 ;
}

static inline char* msg_build_hello_head(char *msg, unsigned int hello_seq,
  unsigned int local_groups_list_seq,
  struct timeval *epoch,
  unsigned int group_count) {
  char *ptr = msg ;
  
  put32(ptr, (unsigned int)MSG_HELLO) ; ptr += 4 ;
  put32(ptr, (unsigned int)hello_seq) ; ptr += 4 ;
  put32(ptr, (unsigned int)local_groups_list_seq); ptr += 4 ;
  put32(ptr, (unsigned int)epoch->tv_sec); ptr += 4 ;
  put32(ptr, (unsigned int)epoch->tv_usec); ptr += 4 ;
  put32(ptr, (unsigned int)group_count) ; ptr += 4 ;
  
  return ptr ;
}

static inline char *msg_build_hello_gid(char *msg, u_int gid) {
  char *ptr = msg;
  
  put32(ptr, (unsigned int)gid); ptr += 4;
  
  return ptr;
}

static inline char* msg_parse_hello_head(char *msg, unsigned int *hello_seq,
  unsigned int *local_groups_list_seq,
  struct timeval *epoch,
  unsigned int *groups_count) {
  char *ptr = msg + 4 ;
  
  *hello_seq = get32(ptr) ; ptr += 4 ;
  *local_groups_list_seq = get32(ptr) ; ptr += 4 ;
  epoch->tv_sec = get32(ptr); ptr += 4 ;
  epoch->tv_usec = get32(ptr); ptr += 4 ;
  *groups_count = get32(ptr) ; ptr += 4 ;
  
  return ptr ;
}

static inline char* msg_parse_hello_gid(char *msg, u_int *gid) {
  char *ptr = msg ;
  
  *gid = get32(ptr); ptr += 4  ;
  
  return ptr ;
}

/* INITIAL_ED message format
 *        4   bytes  type   (message type - MSG_INITIAL_ED)
 *        4   bytes  seq
 *        8   bytes  local_epoch
 *        4   bytes  local_finished
 *        8   bytes  local_sending_ts
 *        4   bytes  remote_finished
 *        8   bytes  remote_sending_ts
 */

/*  #define FINISHED_UNKNOWN      0 */
/*  #define FINISHED_YES          1 */
/*  #define FINISHED_NO           2 */

static inline char *msg_build_initial_ed(char *msg,
  unsigned int seq,
  struct timeval *local_epoch,
  unsigned int local_seq,
  struct timeval *local_sending_ts,
  unsigned int remote_seq,
  struct timeval *remote_sending_ts) {
  char *ptr = msg ;
  put32(ptr, (unsigned int)MSG_INITIAL_ED) ; ptr += 4 ;
  put32(ptr, (unsigned int)seq) ; ptr += 4;
  put32(ptr, (unsigned int)local_epoch->tv_sec); ptr += 4 ;
  put32(ptr, (unsigned int)local_epoch->tv_usec); ptr += 4 ;
  put32(ptr, (unsigned int)local_seq); ptr += 4 ;
  put32(ptr, (unsigned int)local_sending_ts->tv_sec); ptr += 4 ;
  put32(ptr, (unsigned int)local_sending_ts->tv_usec) ; ptr += 4 ;
  put32(ptr, (unsigned int)remote_seq); ptr += 4 ;
  put32(ptr, (unsigned int)remote_sending_ts->tv_sec); ptr += 4 ;
  put32(ptr, (unsigned int)remote_sending_ts->tv_usec); ptr += 4 ;
  
  return ptr ;
}

static inline char *msg_parse_initial_ed(char *msg,
  unsigned int *seq,
  struct timeval *remote_epoch,
  unsigned int *remote_seq,
  struct timeval *remote_sending_ts,
  unsigned int *local_seq,
  struct timeval *local_sending_ts) {
  char *ptr = msg + 4 ;
  *seq = get32(ptr); ptr += 4 ;
  remote_epoch->tv_sec = get32(ptr); ptr += 4 ;
  remote_epoch->tv_usec = get32(ptr); ptr += 4 ;
  *remote_seq = get32(ptr); ptr += 4;
  remote_sending_ts->tv_sec = get32(ptr); ptr += 4 ;
  remote_sending_ts->tv_usec = get32(ptr); ptr += 4 ;
  *local_seq = get32(ptr); ptr += 4;
  local_sending_ts->tv_sec = get32(ptr); ptr += 4 ;
  local_sending_ts->tv_usec = get32(ptr); ptr += 4 ;
  
  return ptr ;
}

/*
 * Validation message format (all fields are network byte order):
 *        4    bytes  type     (message type - MSG_VALIDITY)
 *        4    bytes  valid    (0 - cannot be reached, 1 - could be)
 *        4    bytes  pid      (pid of monitored proc)
 *        4    bytes  host     (IP address of host)
 *        8    bytes  timestamp
 */



#define VAL_POSSIBLE      1
#define VAL_IMPOSSIBLE    0

static inline char *msg_build_val(char *msg, int possible, u_int pid,
  struct sockaddr_in *addr,
  struct timeval *tv) {
  char *ptr = msg ;
  put32(ptr, (unsigned int)MSG_VALIDITY) ; ptr += 4 ;
  put32(ptr, (unsigned int)possible); ptr += 4;
  put32(ptr, (unsigned int)pid); ptr += 4;
  put32(ptr, (unsigned int)ntohl(addr->sin_addr.s_addr)); ptr += 4;
  put32(ptr, (unsigned int)tv->tv_sec); ptr += 4;
  put32(ptr, (unsigned int)tv->tv_usec); ptr += 4;
  
  return ptr;
}

static inline char *msg_parse_val(char *msg, int *possible, u_int *pid,
  struct sockaddr_in *addr,
  struct timeval *tv) {
  
  
  
  char *ptr = msg + 4;
  
  *possible = get32(ptr); ptr += 4;
  *pid = get32(ptr); ptr += 4;
  addr->sin_addr.s_addr = htonl(get32(ptr)); ptr += 4;
  tv->tv_sec = get32(ptr); ptr += 4;
  tv->tv_usec = get32(ptr); ptr += 4;
  
  return ptr;
}

#endif

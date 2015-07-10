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


/* omega_msg.h */
#include "omega_types.h"
#include "msg.h"
#include <netinet/in.h>


/************************************/
/* messages going through pipes     */
/***********************************/

#define OMEGA_FIFO_MSG_LEN (7*4)
#define OMEGA_MAX_FIFO_LEN 100


#define MSG_OMEGA_REGISTER 21
#define MSG_OMEGA_UNREGISTER 22

#define MSG_OMEGA_STARTOMEGA 23
#define MSG_OMEGA_STOPOMEGA 24

#define MSG_OMEGA_GET_LEADER 25
#define MSG_OMEGA_INTERRUPT_MODE 26
#define MSG_OMEGA_NOTIFY 27


#define MSG_OMEGA_RESULT 28
#define MSG_OMEGA_EXT_RESULT 29

/****************************************/
/* messages going through network links */
/****************************************/

#define MSG_OMEGA_ACCUSATION 30



/**********************************************************************/
/* procedures to build and parse messages going through pipes         */
/**********************************************************************/

/*
 * Registration message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_REGISTER)
 *	4    bytes  pid		(process ID)
 */

static inline char *msg_omega_build_reg(char *msg, u_int pid)
{
  char *ptr = msg;
  put32(ptr, (unsigned int)MSG_OMEGA_REGISTER);  ptr += 4;
  put32(ptr, (unsigned int)pid); ptr += 4;
  return ptr;
}

static inline char *msg_omega_parse_reg(char *msg, u_int *pid) {
  char *ptr = msg + 4;
  *pid = get32(ptr); ptr += 4;
  return ptr;
}


/*
 * START OMEGA message format (all fields are network byte order):
 *	4    bytes  type	   (message type - MSG_OMEGA_STARTOMEGA)
 *	4    bytes  gid		   (group ID)
 *  4	 bytes  candidate  (Do I want to be a candidate to the leader election?)
 *  4    bytes  notif_type (Interrupt on any change or query)
 *  4    bytes  Tdu
 *  4	 bytes  TmU
 *  4	 bytes  TmrL
 */

static inline char *msg_omega_build_startomega(char *msg, unsigned int gid, int candidate,
  int notif_type, unsigned int TdU,
unsigned int TmU, unsigned int TmrL)
{
  char *ptr = msg;
  put32(ptr, (unsigned int)MSG_OMEGA_STARTOMEGA);  ptr += 4;
  put32(ptr, (unsigned int)gid); ptr += 4;
  put32(ptr, (unsigned int)candidate); ptr += 4;
  put32(ptr, (unsigned int)notif_type); ptr += 4;
  put32(ptr, (unsigned int)TdU); ptr += 4;
  put32(ptr, (unsigned int)TmU); ptr += 4;
  put32(ptr, (unsigned int)TmrL); ptr += 4;
  return ptr;
}


static inline char *msg_omega_parse_startomega(char *msg, u_int *gid, int *candidate,
  int *notif_type, u_int *TdU, u_int *TmU,
  u_int *TmrL) {
  char *ptr = msg + 4;
  *gid = get32(ptr); ptr += 4;
  *candidate = get32(ptr); ptr += 4;
  *notif_type = get32(ptr); ptr += 4;
  *TdU = get32(ptr); ptr += 4;
  *TmU = get32(ptr); ptr += 4;
  *TmrL = get32(ptr); ptr += 4;
  return ptr;
}

/*
 * STOP OMEGA message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_OMEGA_STOPOMEGA)
 *	4    bytes  gid		(group ID)
 */

static inline char *msg_omega_build_stopomega(char *msg, u_int gid)
{
  char *ptr = msg;
  put32(ptr, (unsigned int)MSG_OMEGA_STOPOMEGA);  ptr += 4;
  put32(ptr, (unsigned int)gid); ptr += 4;
  return ptr;
}

static inline char *msg_omega_parse_stopomega(char *msg, u_int *gid) {
  char *ptr = msg + 4;
  *gid = get32(ptr); ptr += 4;
  return ptr;
}


/*
 * INTERRUPT MODE message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_OMEGA_INTERRUPT_MODE)
 *  4 	 bytes  notif_type  (OMEGA_INTERRUPT_ANY_CHANGE or OMEGA_INTERRUPT_NONE)
 *	4    bytes  gid		(group ID)
 */

static inline char *msg_omega_build_int(char *msg, int notif_type, u_int gid) {
  char *ptr = msg;
  put32(ptr, (unsigned int)MSG_OMEGA_INTERRUPT_MODE); ptr += 4;
  put32(ptr, (unsigned int)notif_type); ptr += 4;
  put32(ptr, (unsigned int)gid); ptr += 4;
  return ptr;
}

static inline char *msg_omega_parse_int(char *msg, int *notif_type, u_int *gid) {
  char *ptr = msg + 4;
  *notif_type = get32(ptr); ptr += 4;
  *gid = get32(ptr); ptr += 4;
  return ptr;
}


/*
 * Notification message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_OMEGA_NOTIFY)
 *	4    bytes  host	(IP address of host)
 *	4    bytes  pid     (process ID)
 *  4	 bytes  gid     (group ID)
 *  4    bytes  leaderstable
 */

static inline char *msg_omega_build_notify(char *msg, struct sockaddr_in *addr, u_int pid, u_int gid,
  int leaderstable) {
  char *ptr = msg;
  put32(ptr, (unsigned int)MSG_OMEGA_NOTIFY); ptr += 4;
  put32(ptr, (unsigned int)ntohl(addr->sin_addr.s_addr)); ptr += 4;
  put32(ptr, (unsigned int)pid); ptr += 4;
  put32(ptr, (unsigned int)gid); ptr += 4;
  put32(ptr, (int)leaderstable); ptr += 4;
  return ptr;
}

static inline char *msg_omega_parse_notify(char *msg, struct sockaddr_in *addr, u_int *pid, u_int *gid,
  int *leaderstable) {
  char *ptr = msg + 4;
  addr->sin_addr.s_addr = htonl(get32(ptr)); ptr += 4;
  *pid = get32(ptr); ptr += 4;
  *gid = get32(ptr); ptr += 4;
  *leaderstable = get32(ptr); ptr += 4;
  return ptr;
}


/*
 * Get leader message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_OMEGA_GET_LEADER)
 *	4    bytes  gid     (group ID)
 */

static inline char *msg_omega_build_getleader(char *msg, u_int gid) {
  char *ptr = msg;
  put32(ptr, (unsigned int)MSG_OMEGA_GET_LEADER); ptr += 4;
  put32(ptr, (unsigned int)gid); ptr += 4;
  return ptr;
}

static inline char *msg_omega_parse_getleader(char *msg, u_int *gid) {
  char *ptr = msg + 4;
  *gid = get32(ptr); ptr += 4;
  return ptr;
}


/*
 * Normal result message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_RESULT)
 *	4    bytes  result
 */

static inline char *msg_omega_build_res(char *msg, int result)
{
  char *ptr = msg;
  
  put32(ptr, (unsigned int)MSG_OMEGA_RESULT);  ptr += 4;
  put32(ptr, (unsigned int)result); ptr += 4;
  
  return ptr;
}

static inline char *msg_omega_parse_res(char *msg, int *result)
{
  char *ptr = msg + 4;
  
  *result = get32(ptr); ptr += 4;
  return ptr;
}


/*
 * Extended result message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_OMEGA_EXT_RESULT)
 *  4    bytes  result
 *	4    bytes  host	(IP address of host)
 *	4    bytes  pid     (process ID)
 *  4    bytes  leader_stable
 */

static inline char *msg_omega_build_ext_res(char *msg, int result, struct sockaddr_in *addr, u_int pid,
  int leaderstable) {
  char *ptr = msg;
  put32(ptr, (unsigned int)MSG_OMEGA_EXT_RESULT); ptr += 4;
  put32(ptr, (unsigned int)result); ptr += 4;
  put32(ptr, (unsigned int)ntohl(addr->sin_addr.s_addr)); ptr += 4;
  put32(ptr, (unsigned int)pid); ptr += 4;
  put32(ptr, (int)leaderstable); ptr += 4;
  return ptr;
}

static inline char *msg_omega_parse_ext_res(char *msg, int *result, struct sockaddr_in *addr, u_int *pid,
  int *leaderstable) {
  char *ptr = msg + 4;
  *result = get32(ptr); ptr += 4;
  addr->sin_addr.s_addr = htonl(get32(ptr)); ptr += 4;
  *pid = get32(ptr); ptr += 4;
  *leaderstable = get32(ptr); ptr += 4;
  return ptr;
}



/******************************************************************************/
/* procedures to build and parse messages going through network links         */
/******************************************************************************/


/*
 * Accusation message format (all fields are network byte order):
 *	4    bytes  type	(message type - MSG_OMEGA_EXT_RESULT)
 *  4	 byte   gid
 *	4    bytes  startTime.tv_sec
 *  4    bytes  startTime.tv_usec
 */

static inline char *msg_omega_build_accusation(char *msg, u_int gid, struct timeval *startTime) {
  char *ptr = msg;
  put32(ptr, (unsigned int)MSG_OMEGA_ACCUSATION); ptr += 4;
  put32(ptr, (unsigned int)gid); ptr += 4;
  put32(ptr, (unsigned int)startTime->tv_sec); ptr += 4;
  put32(ptr, (unsigned int)startTime->tv_usec); ptr += 4;
  return ptr;
}

static inline char *msg_omega_parse_accusation(char *msg, u_int *gid, struct timeval *startTime) {
  char *ptr = msg + 4;
  *gid = get32(ptr); ptr += 4;
  startTime->tv_sec = get32(ptr); ptr += 4;
  startTime->tv_usec = get32(ptr); ptr += 4;
  return ptr;
}

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


/* omega_types.h */

#include <sys/types.h>
#include <netinet/in.h>
#include "list.h"
#include "omega_remote.h"

#define OMEGA_INTERRUPT_NONE	    0
#define OMEGA_INTERRUPT_ANY_CHANGE	1

#define NOT_CANDIDATE 0
#define CANDIDATE 1


struct notif_type_struct {
  int notif_type;
  unsigned int gid;
  int already_notified;    /* says if the process has already been notified for a particular group */
  int candidate;   /* tells if the process is a candidate for the leadership of group gid. */
  struct list_head notif_type_list;
} ;


/* A struct storing processes. */
struct proc_struct {
  struct sockaddr_in addr;
  unsigned int pid;
  struct list_head proc_list;
} ;

/* A struct storing a list of processes for each group */
struct contenders_struct {
  unsigned int gid;
  struct list_head procs_list;
  struct list_head contenders_list;
} ;


/* A struct storing leaders of groups. A leader is composed of
 an address and a pid. */
struct leaders_struct {
  unsigned int gid;
  struct sockaddr_in addr;
  unsigned int pid;
  int stable;   /* Tells if the current leader is stable or not */
  struct list_head leaders_list;
};


struct localregistered_proc_struct {
  
  unsigned int pid;
  int omega_qry_fd;
  int omega_int_fd;
  int omega_cmd_fd;
  struct list_head notif_type_list;  /* type of notification for each group */
  struct list_head localproc_list;
} ;


struct omega_delay_struct {
  char msg[OMEGA_UDP_MSG_LEN] ;
  int msg_len ;
  struct sockaddr_in raddr ;
} ;


#define OMEGA_EVENT_DELAY	    0

struct omega_event_struct {
  int type ;
  struct omega_delay_struct *delay;
  struct timeval tv ;
  struct list_head event_list ;
};

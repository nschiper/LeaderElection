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


/* fdd_type.h */
#ifndef _TYPES_H
#define _TYPES_Hlocal_proc

#include "list.h"

/* The Instantaneous stuff works only for the synchronized clocks */
#ifndef SYNCH_CLOCKS
#ifndef INSTANT_EXPECTED_DELAY_OFF
#define INSTANT_EXPECTED_DELAY_OFF
#endif
#endif

/* FIXME: make this dynamic? */
#define MAX_MSG_LEN 1024

/* The carracteristics of the network */
struct stats_est_struct {
  double e_d ; /* delay estimation */
  double v_d ; /* delay variation */
  double pl ;  /* probability of lose */
} ;

/* A structure used to simulate complete link failure. */
struct link_failure_struct {
  struct timeval next_lf;
  struct timeval end_next_lf;
} ;

struct average_lost_struct {
  unsigned int seq ;
  //  struct timeval sending_ts ;
  struct timeval arrival_ts ;
  struct list_head lost_msg_list ;
} ;


#ifndef INSTANT_LOSS_PROBABILITY_OFF
struct instant_lost_interval_struct {
  struct timeval begin_interval_ts ;
  struct timeval interval_ts ;
  unsigned int lost_messages ;
  unsigned int good_messages ;
  unsigned int begin_seq ;
  struct list_head interval_list ;
} ;
#endif

struct average_delay_struct {
  unsigned int seq ;
  //  unsigned int delay ;
  struct timeval delay_tv ;
  unsigned int remote_sendint ;
  struct timeval arrival_ts ;
  struct list_head delay_list ;
} ;

#ifndef INSTANT_EXPECTED_DELAY_OFF
struct instant_delay_struct {
  struct timeval arrival_ts ;
  unsigned int delay ;
  struct list_head instant_delay_list ;
} ;
#endif

/*  struct initial_ed_struct { */
/*    unsigned int seq ; */
/*    struct timeval sending_ts ; */
/*    struct timeval arrival_ts ; */
/*    struct list_head initial_ed_list ; */
/*  } ; */


#define FINISHED_UNKNOWN      0
#define FINISHED_YES          1
#define FINISHED_NO           2

struct stats_struct {
#ifndef INSTANT_LOSS_PROBABILITY_OFF
  struct list_head intervals_head ;
#endif
  struct list_head average_lost_head ;
  unsigned int last_interval_length ;
  unsigned int current_interval_length ;
  
  unsigned int nb_average_lost_msg ;
  unsigned int nb_average_total_msg ;
  unsigned int last_average_seq ;
  
#ifndef INSTANT_EXPECTED_DELAY_OFF
  struct list_head instant_delay_msg_head ;
  unsigned int nb_instant_delay_msg ;
#endif
  
  unsigned int local_initial_finished ;
  unsigned int remote_initial_finished ;
  unsigned int last_seq ;
  struct timeval expected_arrival_ts ;
  
  struct list_head average_delay_msg_head ;
  unsigned int nb_average_delay_msg ;
  int nb_average_recompute_period ;
  
  struct stats_est_struct est ;
} ;

struct host_struct {
  struct sockaddr_in addr ;
  struct timeval remote_epoch ; /* timestamp of the starting remote host */
  /*  struct timeval epoch ;*/ /* timestamp trusting the remote host */
  
  struct stats_struct stats ;
  
  struct link_failure_struct sim_lf ;
  
  unsigned int local_initial_ed_seq ; /* seq number of the last INITIAL_ED msg sent. */
  
  unsigned int remote_initial_ed_seq ; /* seq number of the last INITIAL_ED msg rcvd. */
  
  struct timeval sending_ts ;	/* timestamp of last received report */
  
  unsigned int hello_seq ;      /* seq number of the last hello multicast
   msg received from this host */
  
  /*    unsigned int seq ;		 *//* seq number of last report */
  unsigned int local_seq ;      /* local seq at the last report time */
  
  struct timeval last_report_ts ; /* timestamp of last sent report */
  struct timeval next_report_ts ; /* timestamp of next report */
  
  struct timeval local_largest_group_ts_rcvd; /* The largest local group
   timestamp received from that host. */
  struct timeval remote_largest_group_ts_rcvd; /* The largest remote group
   timestamp received from that host. */
  
  unsigned int remote_actual_sendint ;
  unsigned int remote_needed_sendint ;
  unsigned int local_needed_sendint ;
  
  struct list_head remote_host_list ;
  
  /* list of remote servers last received from that host */
  struct list_head remote_servers_proc_head ;
  /* list of local clients that monitor this host */
  struct list_head local_clients_proc_head ;
  /* list of local  processes that are monitored by this remote host */
  struct list_head local_servers_proc_head ;
  
  /* list of jointly groups between the local host and this host */
  struct list_head jointly_groups_head ;
  /* list of non_jointly_groups between the local host and this host */
  struct list_head remote_all_groups_head ;
  /* list of the remote processes last received, that are in the jointly groups */
  struct list_head remote_all_groups_procs_head ;
  
  /* list of all the remote processes last received, that are in the jointly groups.
   This list is used to calculate the remote needed sendint */
  struct list_head list_remote_procs_in_groups_to_calc_eta;
  
  unsigned int last_local_clients_list_seq ; /* client list seq. number of last
   parsed process list */
  unsigned int local_clients_list_seq ;  /* seq of the local clients list  */
  unsigned int local_servers_list_seq ;  /* seq of the local servers list  */
  
  unsigned int remote_clients_list_seq ; /* seq of the remote client list  */
  unsigned int remote_servers_list_seq ; /* seq of the remote servers list */
  
  unsigned int remote_groups_list_seq ; /* seq. nb. of the remote groups list */
  unsigned int remote_groups_multicast_list_seq ;
} ;

/* generic list of u_int values */
struct uint_struct {
  unsigned val ;
  struct list_head uint_list ;
} ;


/* Added for Omega */
/* List containing for all the group g of all the local processes p if p
 is visible in g */
struct pid_gid_visibility_struct {
  u_int pid;
  struct list_head pid_list_head;
  struct list_head gid_list_head;
} ;

/* Added for Omega */
/* A list of groups in which a particular process is visisble */
struct glist_struct {
  u_int gid;
  struct list_head gcell;
} ;


/* Added for Omega */
/* A list of timestamps, one for each group */
struct group_ts_struct {
  u_int gid;
  struct timeval ts;
  struct list_head group_ts_list;
} ;


/* Te QoS specification */
struct qos_struct {
  unsigned int TdU ; /* The Upper Bound of the Detection Time */
  unsigned int TmU ; /* The Upper Bound of the Mistake Time */
  unsigned int TmrL ;/* The Lower Bound of the Mistake Reccurence Time */
} ;

struct procqos_struct {
  unsigned int pid ;
  unsigned int int_type ;
  struct sockaddr_in addr ;
  struct qos_struct qos ;
  struct list_head pqlist ;
} ;

struct groupqos_struct {
  unsigned int gid ;
  unsigned int int_type ;
  struct qos_struct *qos ; /* when qos == NULL is not monitoring */
  struct list_head gqlist ;
} ;

struct procgroup_struct {
  unsigned int pid ;
  unsigned int gid ;
  struct list_head pglist ;
} ;

struct trust_struct {
  unsigned int pid ;
  unsigned int int_type ;
  struct uint_struct *gid ;
  struct host_struct *host ;
  struct timeval fresh ;
  struct list_head trust_list ;
} ;

#define INTERRUPT_NONE	        0
#define INTERRUPT_ANY_CHANGE	1
#define INT_SET_FAIL	        2

struct localproc_struct {
  unsigned int pid ;
  struct list_head pqlist_head ;  /* remote processes interested in this process */
  struct list_head gqlist_head ;
  struct list_head trust_list_head ;
  struct list_head tmp_tlist_head ;
  struct list_head local_procs_list ;
} ;

struct delay_struct {
  char msg[MAX_MSG_LEN] ;
  int msg_len ;
  struct sockaddr_in raddr ;
} ;

#define EVENT_NONE	    0
#define EVENT_REPORT	    1
#define EVENT_SUSPECT	    2
#define EVENT_DELAY	    3
#define EVENT_HELLO         4
#define EVENT_INITIAL_ED    5
#define EVENT_SUSPECT_GROUP 6

struct event_struct {
  int type ;
  struct localproc_struct *lproc ;
  struct trust_struct *tproc ;
  struct delay_struct *delay ;
  struct host_struct *host ;
  struct uint_struct *remote_group ;
  struct timeval tv ;
  struct list_head event_list ;
};

struct mng_host_struct {
  struct sockaddr_in addr ;
  struct list_head mng_host_list ;
} ;

#endif









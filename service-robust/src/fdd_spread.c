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


/* spread.c This module deals with the spreading of the information concerning the
 groups. It sends IP multicast messages to all the member of the IP multicast group. */

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "fdd.h"
#include "misc.h"

#ifdef LOG
extern FILE *flog ;
#endif

static unsigned int local_hello_seq ;

extern struct timeval local_epoch ;
extern unsigned int local_groups_list_seq ;
extern unsigned int local_groups_multicast_list_seq ;
extern unsigned int local_procs_groups_list_seq ;
extern struct list_head local_groups_list_head ;

/* Variable initialization */
extern void local_init_multicast() {
  int retval ;
  local_hello_seq = 0 ;
  
  /* send hello multicast now */
  retval = sched_hello_multicast(&local_epoch, 0) ;
  
  if( retval < 0 ) {
#ifdef OUTPUT
    fprintf(stdout, "Could not schedule multicast event\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Could not schedule multicast event\n") ;
#endif
  }
}


/* builds a HELLO type message and multicast it */
extern void send_hello_multicast(struct timeval *now_hello) {
  char msg[MAX_MSG_LEN] ;
  char *ptr = NULL, *ptr_head = NULL, *ptr_groups_list = NULL ;
  struct list_head *tmp_group = NULL ;
  int groups_count = 0 ;
  struct list_head groups_list ;
  
  struct uint_struct *group = NULL ;
  
  int retval ;
  
  ptr = msg ;
  
  ptr_head = ptr ;
  
  if((ptr = msg_skip_hello_head(ptr)) > msg + MAX_MSG_LEN) {
#ifdef OUTPUT
    fprintf(stdout, "local_send_hello_multicast:Buffer overflow\n") ;
#endif
#ifdef LOG
    fprintf(flog, "local_send_hello_multicast:Buffer overflow\n") ;
#endif
    goto out;
  }
  
  
  ptr_groups_list = ptr ;
  /* build the list of local groups in the groups_list variable */
  retval = build_groups_list(&groups_list) ;
  if(retval < 0)
    goto out ;
  
  retval = -EMSGSIZE ;
  groups_count = 0 ;
  /* put each group id in the message */
  list_for_each(tmp_group, &groups_list) {
    group = list_entry(tmp_group, struct uint_struct, uint_list) ;
    ptr = msg_build_hello_gid(ptr, group->val) ;
    
    if(ptr > msg + MAX_MSG_LEN) {
#ifdef OUTPUT
      fprintf(stdout, "local_send_hello_multicast:Buffer overflow\n") ;
#endif
#ifdef LOG
      fprintf(flog, "local_send_hello_multicast:Buffer overflow\n") ;
#endif
      goto out ;
    }
    
    groups_count++ ;
    tmp_group = tmp_group->prev ;
    list_del(&group->uint_list) ;
    free(group) ;
  }
  
  /* multicast message sequence number - used to deal with out of order messages */
  local_hello_seq++ ;
  msg_build_hello_head(ptr_head, local_hello_seq, local_groups_multicast_list_seq,
  &local_epoch, groups_count) ;
  
  if(comm_hello_multicast(msg, ptr-msg) < 0) {
#ifdef OUTPUT
    fprintf(stdout, "Could not send the multicast hello\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Could not send the multicast hello\n") ;
#endif
  }
  else {
#ifdef OUTPUT
    fprintf(stdout, "Said HELLO to everybody\n") ;
#endif
#ifdef LOG_EXTRA
    fprintf(flog, "Said HELLO to everybody\n") ;
#endif
  }
  out:
  return ;
}

/* merge a new multicast message received */
extern void hello_multicast_merge(char *msg, int msg_len, struct sockaddr_in *raddr,
  struct timeval *arrival_ts) {
  
  struct list_head remote_all_groups ;
  struct list_head *tmp ;
  unsigned int hello_seq  ;
  unsigned int remote_groups_list_seq ;
  struct timeval remote_epoch ;
  unsigned int groups_count ;
  int entry_exists = 0;
  
  char *ptr = NULL, *ptr_groups_list = NULL ;
  int retval ;
  
  struct host_struct *host = NULL ;
  int remote_host_already_known = 1, new_group = 0, send_report_msg = 0;
  struct list_head *tmp_jointly;
  struct uint_struct *group;
  
  LIST_HEAD(jointly_groups_bckup);
  
  /* Added for Omega */
  /* Used to know if we already knew that host */
  host = locate_host(raddr);
  
  if (host == NULL)
    remote_host_already_known = 0;
  
  host = NULL;
  
  ptr_groups_list = msg_parse_hello_head(msg, &hello_seq, &remote_groups_list_seq,
  &remote_epoch, &groups_count) ;
#ifdef OUTPUT_EXTRA
  fprintf(stdout, "HELLO received from %u.%u.%u.%u; hello_seq=%u, remote_epoch=%ld.%ld\n",
  NIPQUAD(raddr), hello_seq, remote_epoch.tv_sec, remote_epoch.tv_usec) ;
#endif
#ifdef LOG_EXTRA
  fprintf(flog, "HELLO received from %u.%u.%u.%u; hello_seq=%u, remote_epoch=%ld.%ld\n",
  NIPQUAD(raddr), hello_seq, remote_epoch.tv_sec, remote_epoch.tv_usec) ;
#endif
  
  retval = -ENOMEM ;
  host = locate_create_host(raddr, &remote_epoch, 0, arrival_ts) ;
  if(NULL == host)
    goto out ;
  
  retval = 0 ;
  if(!greater_than(hello_seq, host->hello_seq)) {
    
#ifdef OUTPUT
    fprintf(stdout, "HELLO_OUT_OF_ORDER\n") ;
#endif
#ifdef LOG
    fprintf(flog, "HELLO_OUT_OF_ORDER\n") ;
#endif
    goto out ;
  }
  
  host->hello_seq = hello_seq ;
  
  ptr = ptr_groups_list ;
  
  retval = 0 ;
  
  /*    if(greater_than(host->remote_groups_multicast_list_seq, remote_groups_list_seq) && */
  /*       !list_empty(&host->remote_all_groups_head)) */
  /*      goto out ; */
  
  INIT_LIST_HEAD(&remote_all_groups) ;
  
  retval = 0 ;
  ptr = parse_proc_list(msg, ptr, groups_count, &remote_all_groups, &retval) ;
  if(retval < 0)
    goto out_free_proc_list ;
  
  list_for_each(tmp, &remote_all_groups) {
    struct uint_struct *entry_ptr = NULL ;
    int entry_exists ;
    struct uint_struct *group = NULL ;
    
    group = list_entry(tmp, struct uint_struct, uint_list) ;
    list_insert_ordered(group->val, val, &host->remote_all_groups_head,
    struct uint_struct, uint_list, <) ;
    if(entry_ptr == NULL)
      goto out_free_proc_list ;
    
    retval = sched_suspect_remote_group(host, entry_ptr, arrival_ts) ;
    if(retval < 0)
      goto out_free_proc_list ;
    
    tmp = tmp->prev;
    list_del(&group->uint_list) ;
    free(group) ;
  }
  
  /* Modified for Omega */
  /* Instead of inserting the groups, we first erase the old list and
   insert the new groups. */
  /*list_swap(&remote_all_groups, &host->remote_all_groups_head);
   list_free(&remote_all_groups, struct uint_struct, uint_list);*/
  
  
  
  /* Added for Omega */
  /* Back-up old jointly_groups list of that host */
  {
    struct uint_struct *entry_ptr ;
    
    entry_ptr = NULL;
    entry_exists = 0;
    retval = -ENOMEM;
    
    list_for_each(tmp_jointly, &host->jointly_groups_head) {
      group = list_entry(tmp_jointly, struct uint_struct, uint_list);
      list_insert_ordered(group->val, val, &jointly_groups_bckup, struct uint_struct,
      uint_list, <) ;
      if (entry_ptr == NULL)
        goto out;
    }
  }
  
  retval = build_jointly_groups_list(host, &host->remote_all_groups_head) ;
  
  if( retval < 0 )
    goto out_free_proc_list ;
  
  
  /* Added for Omega */
  /* Accelerate step 2 of the handshake when a FDD sends us a hello message.
  There are two cases:
  1) If we didn't know that host and if we have groups in common, we send a hello
  immediately.
  2) If we knew that host but that host has a new group that we have too,
  we send a hello message immediately.
  
  Step 3 is also accelerated:
  If we received a hello message that contains a new group in which we are
   and if we want to monitor that particular group, then we send a report msg. */
  list_for_each(tmp_jointly, &host->jointly_groups_head) {
    group = list_entry(tmp_jointly, struct uint_struct, uint_list);
    if (!is_in_ordered_list(group->val, &jointly_groups_bckup)) {
      new_group = 1;
      /* There's a new group we are part of, do we want to monitor that group?
       If yes then send a report msg. */
      if(exists_report_in_event_list(raddr, group->val)) {
        send_report_msg = 1;
        break;
      }
    }
  }
  
  if ((!remote_host_already_known && !list_empty(&host->jointly_groups_head)) ||
    (remote_host_already_known && new_group)) {
#ifdef OUTPUT
    fprintf(stdout, "Handshake step 2: we hello back!\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Handshake step 2: we hello back!\n") ;
#endif
    sched_hello_multicast(arrival_ts, 0);
  }
  else {
#ifdef OUTPUT
    fprintf(stdout, "Handshake step 2: we don't hello back!\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Handshake step 2: we don't hello back!\n") ;
#endif
  }
  
  
  if (send_report_msg) {
    if ((host->stats.local_initial_finished == FINISHED_YES) &&
      (host->stats.remote_initial_finished == FINISHED_YES)) {
#ifdef OUTPUT
      fprintf(stdout, "Handshake step 3 case a) (monitor group before hello back): send a report msg now!\n") ;
#endif
#ifdef LOG
      fprintf(flog, "Handshake step 3 case a) (monitor group before hello back): send a report msg now!\n") ;
#endif
      sched_report_now(host, arrival_ts);
    }
  }
  
  host->remote_groups_multicast_list_seq = remote_groups_list_seq ;
  local_groups_list_seq++ ;
  recalc_needed_sendint(host, arrival_ts) ;
  local_sched_report_sooner(host->local_needed_sendint, arrival_ts, host) ;
  
  out_free_proc_list:
  /*list_free(&remote_all_groups, struct uint_struct, uint_list);*/
  out:
  
  /* Added for Omega */
  /* Frees the memory used by the jointly_groups_bckup list */
  list_free(&jointly_groups_bckup, struct uint_struct, uint_list) ;
  
  if(retval < 0) {
#ifdef OUTPUT
    fprintf(stdout, "Error in merge hello_multicast\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Error in merge hello_multicast\n") ;
#endif
  }
  
  return ;
}

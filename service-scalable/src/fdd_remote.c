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


/* fdd_remote.h */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include "fdd.h"
#include "variables_exchange.h"
#include "misc.h"
#include "omega.h"
#include <pthread.h>


/* imported stuff */
extern struct list_head local_procs_list_head ;
extern struct list_head local_groups_list_head ;
extern struct timeval local_epoch ;
extern unsigned int local_groups_list_seq ;

#ifdef LOG
extern FILE *flog ;
#endif

LIST_HEAD(remote_host_list_head) ;

/* frees all the allocation of memory to a host */
static void free_host(struct host_struct *host) {
  
  list_free(&host->remote_servers_proc_head, struct uint_struct, uint_list) ;
  list_free(&host->local_clients_proc_head, struct uint_struct, uint_list) ;
  list_free(&host->local_servers_proc_head, struct uint_struct, uint_list) ;
  list_free(&host->jointly_groups_head, struct uint_struct, uint_list) ;
  list_free(&host->remote_all_groups_head, struct uint_struct, uint_list) ;
  list_free(&host->remote_all_groups_procs_head, struct procgroup_struct,
  pglist) ;
  list_free(&host->list_remote_procs_in_groups_to_calc_eta, struct procgroup_struct,
  pglist);
  
  list_free(&host->stats.average_lost_head, struct average_lost_struct,
  lost_msg_list) ;
  
#ifndef INSTANT_EXPECTED_DELAY_OFF
  list_free(&host->stats.instant_delay_msg_head, struct instant_delay_struct,
  instant_delay_list) ;
#endif
  
  list_free(&host->stats.average_delay_msg_head, struct average_delay_struct,
  delay_list) ;
  
  list_del(&host->remote_host_list);
  
  /* Added for Omega */
  /* When we delete a host, we also have to delete its corresponding entry in
   remotevars list (if it exists). */
  free_host_in_remotevars_list(&host->addr);
  
#ifdef OUTPUT
  fprintf(stdout, "Host %u.%u.%u.%u untrusted forever\n",
  NIPQUAD(&host->addr)) ;
#endif
#ifdef LOG
  fprintf(flog, "Host %u.%u.%u.%u untrusted forever\n", NIPQUAD(&host->addr)) ;
#endif
  free(host);
}

/* build the list of the remote servers processes for a remote host and a
 local process */
static int build_proc_local_clients_list(struct localproc_struct *lproc,
  struct host_struct *host) {
  struct list_head *tmp_pqos  = NULL ;
  struct procqos_struct *pqos = NULL ;
  int is_client = 0 ;
  
  /* add all the remote servers for the local process */
  list_for_each(tmp_pqos, &lproc->pqlist_head) {
    pqos = list_entry(tmp_pqos, struct procqos_struct, pqlist) ;
    if( sockaddr_eq(&pqos->addr, &host->addr) ) {
      if( add_local_client(host, lproc) < 0 ) {
        is_client = -1 ;
        goto out ;
      }
      else
        is_client = 1 ;
    }
  }
  out:
  return is_client ;
}

/* builds the list of the remote servers processes for a remote host */
static int build_all_local_clients_list(struct host_struct *host) {
  
  struct list_head *tmp_lproc    = NULL ;
  struct localproc_struct *lproc = NULL ;
  int clients = 0 ;
  
  /* add all the remote servers for each local process */
  list_for_each(tmp_lproc, &local_procs_list_head) {
    lproc = list_entry(tmp_lproc, struct localproc_struct,
    local_procs_list) ;
    if(build_proc_local_clients_list(lproc, host) < 0) {
      clients = -1 ;
      goto out ;
    }
    else
      clients = 1 ;
  }
  if(clients)
    host->local_clients_list_seq++ ;
  
  out:
  return clients ;
}

/* create a new host */
static struct host_struct *create_host(struct sockaddr_in *addr,
  struct timeval *remote_epoch,
  unsigned int seq,
  struct timeval *now) {
  struct host_struct *host = NULL ;
  struct timeval tv ;
  
  host = malloc(sizeof(*host)) ;
  if(host == NULL)
    goto out ;
  
  /* init its IP address */
  addr->sin_port = htons(DEFAULT_PORT) ;
  addr->sin_family = PF_INET ;
  memcpy(&host->addr, addr, sizeof(*addr)) ;
  if(remote_epoch)
    memcpy(&host->remote_epoch, remote_epoch, sizeof(host->remote_epoch)) ;
  else
    timerclear(&host->remote_epoch) ;
  
  /* assume to be now the moment of the last_sending of the remote,
   last_report sent and next_report */
  memcpy(&host->sending_ts, now, sizeof(host->sending_ts)) ;
  memcpy(&host->last_report_ts, now, sizeof(host->last_report_ts)) ;
  memcpy(&host->next_report_ts, now, sizeof(host->next_report_ts)) ;
  
  /* init the statistics */
  stats_init(&host->stats) ;
  
  timerclear(&host->sim_lf.next_lf);
  timerclear(&host->sim_lf.end_next_lf);
  
  host->local_initial_ed_seq = 0;
  host->remote_initial_ed_seq = 0;
  
  host->hello_seq = 0 ;
  host->stats.last_seq = seq ;
  host->local_seq = 0 ;
  host->last_local_clients_list_seq = 0 ;
  host->local_clients_list_seq = 0 ;
  host->local_servers_list_seq = 0 ;
  host->remote_clients_list_seq = 0 ;
  host->remote_servers_list_seq = 0 ;
  host->remote_groups_list_seq = 0 ;
  host->remote_groups_multicast_list_seq = 0 ;
  
  host->remote_actual_sendint = MAX_SENDINT ;
  host->remote_needed_sendint = MAX_SENDINT ;
  host->local_needed_sendint = MAX_SENDINT ;
  
  timerclear(&host->local_largest_group_ts_rcvd);
  timerclear(&host->remote_largest_group_ts_rcvd);
  
  INIT_LIST_HEAD(&host->local_servers_proc_head) ;
  INIT_LIST_HEAD(&host->local_clients_proc_head) ;
  INIT_LIST_HEAD(&host->remote_servers_proc_head) ;
  INIT_LIST_HEAD(&host->jointly_groups_head) ;
  INIT_LIST_HEAD(&host->remote_all_groups_head) ;
  INIT_LIST_HEAD(&host->remote_all_groups_procs_head) ;
  INIT_LIST_HEAD(&host->list_remote_procs_in_groups_to_calc_eta);
  
  list_add(&host->remote_host_list, &remote_host_list_head);
  
  /* build the list of remote servers needed from the remote
   host by the local clients */
  if( build_all_local_clients_list(host) < 0 ) {
    
#ifdef OUTPUT
    fprintf(stdout, "Could not build the clients lists for host:%u.%u.%u.%u\n",
    NIPQUAD(&host->addr) ) ;
#endif
    
#ifdef LOG
    fprintf(flog, "Could not build the clients lists for host:%u.%u.%u.%u\n",
    NIPQUAD(&host->addr) ) ;
#endif
    
    goto free_host ;
  }
  
  timerclear(&tv) ;
  /* begin a message interchange period in which the message's delay standard
   deviation is computed */
  send_initial_ed_msg(host, FINISHED_UNKNOWN, &tv, now) ;
  
  goto out ;
  
  free_host:
  free_host(host) ;
  host = NULL ;
  
  out:
  return host;
}

/* locate the qos specification needed by a local process
 from a remote server */
/*  static struct qos_struct *locate_qos(struct localproc_struct *lproc,  */
/*  				     u_int pid, struct sockaddr_in *addr) { */
/*    struct list_head *tmp; */
/*    struct procqos_struct *pq; */

/*    list_for_each(tmp, &lproc->pqlist_head) { */
/*      pq = list_entry(tmp, struct procqos_struct, pqlist); */
/*      if(pq->pid == pid && sockaddr_eq(&pq->addr, addr)) */
/*        return &pq->qos ; */

/*      if(pq->pid > pid) */
/*        break ; */

/*    } */

/*    return NULL ; */
/*  } */

/* parse the list of processes received in a report message
 from a remote host */
extern char *parse_proc_list(char *msg, char *ptr_start, int pcount,
  struct list_head *list, int *retval) {
  int i ;
  char *ptr = ptr_start ;
  unsigned int pid ;
  
  struct uint_struct *entry_ptr ;
  int entry_exists ;
  
  entry_ptr = NULL ;
  
  INIT_LIST_HEAD(list) ;
  
  for( i = 0 ; i < pcount ; i++) {
    ptr = msg_parse_rep_pid(ptr, &pid) ;
    
    *retval = -EMSGSIZE ;
    if (ptr > msg + MAX_MSG_LEN)
      goto out ;
    
    entry_ptr = NULL ;
    /* insert each value in the list given as argument */
    list_insert_ordered(pid, val, list, struct uint_struct, uint_list, <) ;
    *retval = -ENOMEM ;
    if(entry_ptr == NULL)
      goto out ;
  }
  *retval = 0 ;
  out:
  if(*retval < 0)
    list_free(list, struct uint_struct, uint_list) ;
  return ptr ;
}

/* Added for Omega*/
/* Parse the remotevars list received and insert them in the remotevars list */
static char *parse_and_insert_remotevars_list(char *msg, char *ptr_start, unsigned int remotevars_count,
  struct sockaddr_in *addr, int *retval) {
  int i;
  char *ptr = ptr_start ;
  unsigned int gid ;
  struct timeval accusationTime, startTime;
  
  for(i=0 ; i < remotevars_count ; i++) {
    ptr = msg_parse_rep_localvars(ptr, &gid, &accusationTime, &startTime);
    
    *retval = -EMSGSIZE ;
    if (ptr > msg + MAX_MSG_LEN)
      goto out ;
    
    if (omega_group_exists_locally(gid, NOT_CANDIDATE)) {
      if (insert_in_remotevars(addr, gid, &accusationTime, &startTime) < 0)
        fprintf(stderr, "Couldn't allocate new memory when receiving remote variables in an alive message\n");
    }
  }
  *retval = 0 ;
  out:
  if(*retval < 0) {
#ifdef OUTPUT
    fprintf(stdout, "parse_and_insert_remotevars_list: Buffer overflow\n");
#endif
#ifdef LOG
    fprintf(flog, "parse_and_insert_remotevars_list: Buffer overflow\n");
#endif
  }
  return ptr ;
}


/* you must first init p->tmp_tlist_head to empty */
/* build the list of trusted processes for a local process */
static int build_proc_trust_list(struct localproc_struct *lproc,
  struct host_struct *rhost,
  struct list_head *proc_list,
  struct timeval *sending_ts,
  struct timeval *arrival_ts) {
  struct list_head *tmp_q = NULL ;
  struct uint_struct *remote_proc_pid = NULL ;
  struct trust_struct *trust_proc = NULL ;
  struct procqos_struct *pqos = NULL ;
  struct timeval fresh ;
  int retval ;
#ifndef SYNCH_CLOCKS
  unsigned int alfa ;
#endif
  
  list_for_each(tmp_q, proc_list) {
    remote_proc_pid = list_entry(tmp_q, struct uint_struct, uint_list) ;
    pqos = locate_pqos(lproc, remote_proc_pid->val, &rhost->addr) ;
    /* permanently suspect if no qos is specified */
    if( NULL == pqos || 0 == pqos->qos.TdU )
      continue ;
    
    /* set the fresh time of the remote process */
#ifdef SYNCH_CLOCKS
    unit2timer(pqos->qos.TdU, &fresh) ;
    timeradd(sending_ts, &fresh, &fresh) ;
#else
    alfa = pqos->qos.TdU - wei_sendint(&pqos->qos, &rhost->stats) ;
    unit2timer(alfa, &fresh) ;
    timeradd(&rhost->stats.expected_arrival_ts, &fresh, &fresh) ;
#endif
    
    trust_proc = malloc(sizeof(*trust_proc)) ;
    retval = -ENOMEM ;
    if(NULL == trust_proc)
      goto out ;
    
    trust_proc->gid = NULL ;
    trust_proc->int_type = pqos->int_type ;
    trust_proc->pid = remote_proc_pid->val ;
    trust_proc->host = rhost ;
    memcpy(&trust_proc->fresh, &fresh, sizeof(fresh)) ;
    
    list_add(&trust_proc->trust_list, &lproc->tmp_tlist_head) ;
  }
  
  retval = 0 ;
  out:
  return retval ;
}

/* build the trust lists for all local clients of the remote host */
static int build_trust_lists(struct host_struct *rhost,
  struct list_head *proc_list,
  struct timeval *sending_ts,
  struct timeval *arrival_ts) {
  
  struct list_head *tmp_p                  = NULL ;
  struct uint_struct *plocal_client_pid    = NULL ;
  struct localproc_struct *lproc           = NULL ;
  int retval ;
  
  retval = 0 ;
#ifndef SYNCH_CLOCKS
  /* skip if the expected arrival time could not be initialized */
  if( !timerisset(&rhost->stats.expected_arrival_ts) )
    goto out ;
#endif
  
  /* for each process which is client of the remote host */
  list_for_each(tmp_p, &rhost->local_clients_proc_head) {
    plocal_client_pid = list_entry(tmp_p, struct uint_struct, uint_list) ;
    
    lproc = get_local_proc(plocal_client_pid->val) ;
    /* build the trust list for the local process */
    retval = build_proc_trust_list(lproc, rhost, proc_list,
    sending_ts, arrival_ts) ;
    if (retval < 0)
      goto out ;
  }
  retval = 0 ;
  
  out:
  return retval ;
}

/* parse a received report message and builds a list of remote
 processes and their groups to which they belong */
static char *parse_group_procs_list(char *msg, char *ptr_start,
  struct sockaddr_in *addr, unsigned int gid,
  int pcount, struct list_head *list, unsigned int eta_rcvd,
  int *retval, struct timeval *arrival_ts) {
  int i ;
  char *ptr = ptr_start ;
  unsigned int pid ;
  
  struct procgroup_struct *entry_ptr ;
  int entry_exists ;
  
  INIT_LIST_HEAD(list) ;
  
  for(i = 0 ; i < pcount ; i++) {
    ptr = msg_parse_rep_pid(ptr, &pid) ;
    
    *retval = -EMSGSIZE ;
    if(ptr > msg + MAX_MSG_LEN)
      goto out ;
    
    
    entry_ptr = NULL ;
    list_insert_ordered(pid, pid, list, struct procgroup_struct, pglist, <) ;
    *retval = -ENOMEM ;
    if( entry_ptr == NULL )
      goto out ;
    
    entry_ptr->gid = gid ;
    
    /* Added for Omega */
    /* Upon received alive */
    if (omega_group_exists_locally(gid, NOT_CANDIDATE) && eta_rcvd) {
      if (add_proc_in_globalContenders_set(addr, pid, gid) < 0) {
        fprintf(stderr, "omega error: upon receiving alive msg from: %u.%u.%u.%u impossible\n",  NIPQUAD(addr));
        fprintf(stderr, "to add proc: %u in contenders set of group: %u\n", pid, gid);
      }
    }
    
  }
  *retval = 0 ;
  out:
  if(*retval < 0)
    list_free(list, struct procgroup_struct, pglist) ;
  return ptr ;
}

/* build the trust list for a local process and the servers belonging
 to the given group */
static int build_proc_trust_list_group(unsigned int gid,
  struct localproc_struct *lproc,
  struct host_struct *rhost,
  struct list_head *procgroup_list,
  struct timeval *sending_ts,
  struct timeval *arrival_ts) {
  struct list_head *tmp_pg = NULL ;
  struct procgroup_struct *remote_procgroup = NULL ;
  struct trust_struct *trust_proc = NULL ;
  struct timeval fresh ;
  struct groupqos_struct *gqos = NULL ;
  int retval = 0 ;
#ifndef SYNCH_CLOCKS
  unsigned int alfa ;
#endif
  
  /* find out the qos specif. for the given group */
  gqos = locate_gqos(lproc, gid) ;
  /* permanently suspect if no qos is specified */
  if(NULL == gqos || gqos->qos == NULL || gqos->qos->TdU == 0)
    return 0 ;
  
  /* find out all the remote processes in the list belonging to the
   given group */
  list_for_each(tmp_pg, procgroup_list) {
    remote_procgroup = list_entry(tmp_pg, struct procgroup_struct, pglist) ;
    
    if(remote_procgroup->gid != gid)
      continue ;
    
#ifdef SYNCH_CLOCKS
    unit2timer(gqos->qos->TdU, &fresh) ;
    timeradd(sending_ts, &fresh, &fresh) ;
#else
    /* set the next freshness point for the suspicion of the process */
    alfa = gqos->qos->TdU - wei_sendint(gqos->qos, &rhost->stats) ;
    unit2timer(alfa, &fresh) ;
    timeradd(&rhost->stats.expected_arrival_ts, &fresh, &fresh) ;
#endif
    
    trust_proc = malloc(sizeof(*trust_proc)) ;
    
    retval = -ENOMEM ;
    if(NULL == trust_proc)
      goto out ;
    
    retval = -ENOMEM ;
    trust_proc->gid = malloc(sizeof(*(trust_proc->gid))) ;
    if(NULL == trust_proc->gid) {
      free(trust_proc) ;
      goto out ;
    }
    
    trust_proc->pid = remote_procgroup->pid ;
    trust_proc->int_type = gqos->int_type ;
    trust_proc->gid->val = gid ;
    trust_proc->host = rhost ;
    memcpy(&trust_proc->fresh, &fresh, sizeof(fresh)) ;
    
    list_add(&trust_proc->trust_list, &lproc->tmp_tlist_head) ;
  }
  retval = 0 ;
  out:
  return retval ;
}

/* build the trust list for all local processes and for all groups */
static int
build_all_procs_trust_lists_all_groups(struct host_struct *rhost,
  struct list_head *procgroup_list,
  struct timeval *sending_ts,
  struct timeval *arrival_ts) {
  struct list_head *tmp_jointly = NULL ;
  struct uint_struct *group = NULL ;
  
  struct list_head *tmp_lproc = NULL ;
  struct localproc_struct *lproc = NULL ;
  
  int retval ;
  
  retval = 0 ;
  
#ifndef SYNCH_CLOCKS
  /* give up if expected arrival could not be determined */
  if( !timerisset(&rhost->stats.expected_arrival_ts) )
    goto out ;
#endif
  
  /* for each jointly group */
  list_for_each(tmp_jointly, &rhost->jointly_groups_head) {
    group = list_entry(tmp_jointly, struct uint_struct, uint_list) ;
    /* for each local process */
    list_for_each(tmp_lproc, &local_procs_list_head) {
      lproc = list_entry(tmp_lproc, struct localproc_struct,
      local_procs_list) ;
      /* build the trust list of the local process for
       the group choosed before */
      retval = build_proc_trust_list_group(group->val, lproc,
        rhost, procgroup_list,
        sending_ts,
      arrival_ts) ;
      if(retval < 0)
        goto out ;
    }
  }
  
  retval = 0 ;
  out:
  return retval ;
}

/* does not compare hosts, already guaranteed to be the same
 * gid is also the same (not even present in trust_struct */
static int trust_member(struct trust_struct *tproc,
  struct list_head *trust_list) {
  struct list_head *tmp ;
  struct trust_struct *entry ;
  
  list_for_each(tmp, trust_list) {
    entry = list_entry(tmp, struct trust_struct, trust_list) ;
    if(entry->pid == tproc->pid) {
      if(entry->gid == NULL || tproc->gid == NULL) {
        if( entry->gid == tproc->gid )
          return 1 ;
      }
      else {
        if( (entry->gid->val) == (tproc->gid->val) )
          return 1 ;
      }
    }
  }
  return 0 ;
}

/* commit the trust list. since now they were only temporar */
static void commit_proc_trust_list(struct localproc_struct *lproc,
  struct host_struct *rhost,
  struct timeval *arrival_ts) {
  struct list_head *tmp_trust    = NULL ;
  struct trust_struct *trust     = NULL ;
  struct list_head remove_list          ;
  int error ;
  
  /* move all the trust processes of the remote host in the remove_list */
  local_untrust_host(lproc, rhost, &remove_list);
  
  list_for_each(tmp_trust, &remove_list) {
    trust = list_entry(tmp_trust, struct trust_struct, trust_list);
    /* for each process which was trusted send a CRASH notification if
     it does not pertain to the new list of trusted processes */
    if(INTERRUPT_ANY_CHANGE == trust->int_type &&
      !trust_member(trust, &lproc->tmp_tlist_head)) {
      local_change_notify(lproc, CRASHED_NOTIF, trust, arrival_ts);
    }
  }
  
  while(!list_empty(&lproc->tmp_tlist_head)) {
    trust = list_entry(lproc->tmp_tlist_head.next, struct trust_struct,
    trust_list) ;
    list_del(&trust->trust_list) ;
    
    /* schedule a suspect event for each nearly trusted remote process */
    error = sched_suspect(lproc, trust, &trust->fresh) ;
    
    if(error < 0) {
#ifdef OUTPUT
      fprintf(stderr, "fdd: scheduling suspect event failed.") ;
#endif
      
#ifdef LOG
      fprintf(flog, "fdd: scheduling suspect event failed.") ;
#endif
      if(INTERRUPT_ANY_CHANGE == trust->int_type &&
      trust_member(trust, &remove_list))
      local_change_notify(lproc, CRASHED_NOTIF, trust, arrival_ts) ;
      free_trust(trust) ;
    }
    else {
      /* this process was not in the previous list of trusted processes:
       send a TRUST notification to the client */
      list_add(&trust->trust_list, &lproc->trust_list_head) ;
      if( INTERRUPT_ANY_CHANGE == trust->int_type &&
        !trust_member(trust, &remove_list) ) {
#ifdef OUTPUT
        printf("Sending TRUST_NOTIF to %u, for %u on %u.%u.%u.%u\n",
          lproc->pid, trust->pid,
        NIPQUAD(&trust->host->addr)) ;
#endif
        local_change_notify(lproc, TRUST_NOTIF, trust, arrival_ts);
      }
    }
  }
  list_splice(&remove_list, &lproc->tmp_tlist_head);
}

/* test if a local process joined at least one of the joinlty groups of
 processes with a remote host */
static int is_in_jointly_groups(struct localproc_struct *lproc,
  struct host_struct *rhost) {
  
  struct list_head *tmp_group = NULL ;
  struct uint_struct *group = NULL ;
  
  list_for_each(tmp_group, &rhost->jointly_groups_head) {
    group = list_entry(tmp_group, struct uint_struct, uint_list) ;
    
    if(locate_gqos(lproc, group->val))
      return 1 ;
  }
  return 0 ;
}

/* commit the trust list of all the local clients of a host or
 of these local proceses which monitor a jointly group */
static void commit_trust_lists(struct host_struct *rhost,
  struct timeval *arrival_ts) {
  struct list_head *tmp_p         = NULL ;
  struct localproc_struct *lproc  = NULL ;
  
  list_for_each(tmp_p, &local_procs_list_head) {
    lproc = list_entry(tmp_p, struct localproc_struct, local_procs_list) ;
    
    if(is_in_ordered_list(lproc->pid, &rhost->local_clients_proc_head) ||
    is_in_jointly_groups(lproc, rhost))
    commit_proc_trust_list(lproc, rhost, arrival_ts) ;
  }
}

/* suspect a remote host */
extern void remote_host_suspect(struct host_struct *host,
  struct timeval *now) {
  
  struct list_head *tmp_lproc    = NULL ;
  struct localproc_struct *lproc = NULL ;
  struct list_head *tmp_trust    = NULL ;
  struct trust_struct *trust     = NULL ;
  struct list_head remove_list          ;
  
  
  list_for_each(tmp_lproc, &local_procs_list_head) {
    lproc = list_entry(tmp_lproc, struct localproc_struct, local_procs_list) ;
    
    /* take all the trusts of all the local processes for the remote host */
    local_untrust_host(lproc, host, &remove_list);
    
    list_for_each(tmp_trust, &remove_list) {
      trust = list_entry(tmp_trust, struct trust_struct, trust_list);
      /* send a CRASH notification to each local process
      for each monitored remote process on
         the remote host */
      if(INTERRUPT_ANY_CHANGE == trust->int_type)
        local_change_notify(lproc, CRASHED_NOTIF, trust, now);
      tmp_trust = tmp_trust->prev ;
      list_del(&trust->trust_list) ;
      
      free_trust(trust) ;
    }
  }
  
  /* stop sending report messages to the remote host */
  remove_report_event(host) ;
  /* stop sending initial type message for the
   fast computation of the standard deviation */
  remove_initial_ed_event(host) ;
  /* remove all the suspicion events on the remote groups of the
   remote host */
  remove_host_suspect_group(host) ;
  
  free_host(host) ;
  return ;
}

/* locate a remote host data structure by its IP address */
extern struct host_struct *locate_host(struct sockaddr_in *addr) {
  struct list_head *tmp     = NULL ;
  struct host_struct *rhost = NULL ;
  
  list_for_each(tmp, &remote_host_list_head) {
    rhost = list_entry(tmp, struct host_struct, remote_host_list);
    if(sockaddr_eq(addr, &rhost->addr))
      return rhost;
  }
  return NULL;
}

/* locate a remote host data structure by its IP address and create it
 if it doesn't exist */
extern struct host_struct *locate_create_host(struct sockaddr_in *addr,
  struct timeval *remote_epoch,
  unsigned int seq,
  struct timeval *now) {
  struct host_struct *rhost = NULL ;
  
  
  /* search the host */
  rhost = locate_host(addr) ;
  /* create if doesn't exist */
  if(NULL == rhost) {
    rhost = create_host(addr, remote_epoch, seq, now) ;
    goto out ;
  }
  
  
  if(rhost && !timerisset(&rhost->remote_epoch) && remote_epoch)
    memcpy(&rhost->remote_epoch, remote_epoch, sizeof(rhost->remote_epoch)) ;
  if(rhost && remote_epoch &&
    timercmp(remote_epoch, &rhost->remote_epoch, <)) {
    /* the remote epoch of the sender host of the message is smaller
    than the last received.
    it could be a message sent before the host crashes, so nothing
     will be altered */
#ifdef LOG
    fprintf(flog, "INVALID MESSAGE RECEIVED\n") ;
#endif
    rhost = NULL ;
    goto out ;
  }
  
  if(rhost && remote_epoch &&
    timercmp(remote_epoch, &rhost->remote_epoch, >)) {
    
    /* the remote_epoch of the host is latter than the last received */
    /* host after a crash */
#ifdef OUTPUT
    fprintf(stdout, "HOST AFTER A CRASH\n") ;
#endif
#ifdef LOG
    fprintf(flog, "HOST AFTER A CRASH\n") ;
#endif
    
    /* suspect the host and create a new one */
    remote_host_suspect(rhost, now) ;
    rhost = create_host(addr, remote_epoch, seq, now) ;
  }
  out:
  return rhost;
}

/* compute the sendint value needed for the group
 monitoring of a remote host */
static unsigned int calc_groups_sendint(struct host_struct *rhost) {
  
  struct list_head *tmp_procgroup = NULL ;
  struct list_head *tmp_lproc = NULL ;
  
  struct procgroup_struct *group = NULL ;
  struct localproc_struct *lproc = NULL ;
  struct groupqos_struct *gqos = NULL ;
  
  /* the sendint value will be at most equal to a maximum value */
  unsigned int sendint = MAX_SENDINT ;
  unsigned int sendint1 ;
  
  /* Modified for Omega */
  /* Because there is now the option to be invisible in a group,
  we calculate the needed sendint only for the groups in which the
  remote host is visible and in which we are. A host is visible in group g if we received,
   in the las report msg, a process which is in g in rhost->list_remote_procs_in_groups_to_calc_eta. */
  list_for_each(tmp_procgroup, &rhost->list_remote_procs_in_groups_to_calc_eta) {
    group = list_entry(tmp_procgroup, struct procgroup_struct, pglist) ;
    
    if (!is_in_ordered_list(group->gid, &local_groups_list_head))
      continue;
    else {
      list_for_each(tmp_lproc, &local_procs_list_head) {
        lproc = list_entry(tmp_lproc, struct localproc_struct,
        local_procs_list) ;
        
        gqos = locate_gqos(lproc, group->gid) ;
        if(NULL == gqos || NULL == gqos->qos || gqos->qos->TdU == 0) {
          continue ;
        }
        sendint1 = wei_sendint(gqos->qos, &rhost->stats) ;
        if(sendint1 < sendint)
          sendint = sendint1 ;
      }
    }
  }
  
  return sendint ;
}

/* recompute the sendint value needed from a remote host */
extern void recalc_needed_sendint(struct host_struct *rhost,
  struct timeval *now) {
  unsigned int sendint, sendint1 ;
  struct list_head *tmp_proc        = NULL ;
  struct list_head *tmp_pqos        = NULL ;
  
  struct procqos_struct *pqos       = NULL ;
  struct uint_struct *local_client_pid  = NULL ;
  struct localproc_struct *lproc    = NULL ;
  
  /* check if the initially value for the standard deviation of
   messages was completly determined */
  if(rhost->stats.local_initial_finished != FINISHED_YES ||
  rhost->stats.remote_initial_finished != FINISHED_YES)
  return ;
  
  sendint = calc_groups_sendint(rhost) ;
  
  /* for each local client of the remote host compute its needed
  value for the sendint
   from the remote host */
  list_for_each(tmp_proc, &rhost->local_clients_proc_head) {
    local_client_pid = list_entry(tmp_proc, struct uint_struct, uint_list) ;
    lproc = get_local_proc(local_client_pid->val) ;
    
    list_for_each(tmp_pqos, &lproc->pqlist_head) {
      pqos = list_entry(tmp_pqos, struct procqos_struct, pqlist) ;
      if( sockaddr_eq(&pqos->addr, &rhost->addr) ) {
        /* don't affect sendint if no qos is specified */
        if( pqos->qos.TdU == 0 )
          continue ;
        sendint1 = wei_sendint(&pqos->qos, &rhost->stats) ;
        if(sendint1 < sendint)
          sendint = sendint1 ;
      }
    }
  }
  
  /* the value for the sendint cannot be smaller than a given threshold */
  if (sendint < MIN_SENDINT) {
#ifdef OUTPUT
    fprintf(stdout, "SENDINT=%u TOO SMALL. CORRECTING... SENDINT=%u\n",
    sendint, MIN_SENDINT) ;
#endif
#ifdef LOG
    fprintf(flog, "SENDINT=%u TOO SMALL. CORRECTING... SENDINT=%u\n",
    sendint, MIN_SENDINT) ;
#endif
    sendint = MIN_SENDINT ;
  }
  
  rhost->remote_needed_sendint = sendint;
}

/* build the trust lists for a local processes with respect to a remote host */
extern void remote_replay_host(struct localproc_struct *lproc,
  struct host_struct *host, struct timeval *now) {
  int retval ;
  struct list_head *tmp ;
  struct uint_struct *group ;
  
  INIT_LIST_HEAD(&lproc->tmp_tlist_head) ;
  /* build the trust list for the Point to Point monitoring */
  retval = build_proc_trust_list(lproc, host, &host->remote_servers_proc_head,
  &host->sending_ts, now) ;
  if (retval < 0)
    goto out ;
  
  /* build the trust list for the group monitoring */
  list_for_each(tmp, &host->jointly_groups_head) {
    group = list_entry(tmp, struct uint_struct, uint_list) ;
    
    retval = build_proc_trust_list_group(group->val, lproc, host,
      &host->remote_all_groups_procs_head,
    &host->sending_ts, now) ;
    if(retval < 0)
      goto out ;
  }
  /* commit changes */
  commit_proc_trust_list(lproc, host, now) ;
  
  out:
  list_free(&lproc->tmp_tlist_head, struct trust_struct, trust_list) ;
  
  if (retval < 0) {
#ifdef OUTPUT
    fprintf(stderr, "remote_replay failed\n") ;
#endif
#ifdef LOG
    fprintf(flog, "remote_replay failed\n") ;
#endif
  }
}


static inline int add_proc_group_list2list(u_int gid, struct list_head *list1, struct list_head *list2) {
  
  struct list_head *tmp_head1, *tmp_head2;
  struct procgroup_struct *procgroup1, *procgroup2, *entry_ptr;
  
  if (list_empty(list2)) {
    list_for_each(tmp_head2, list1) {
      procgroup1 = list_entry(tmp_head2, struct procgroup_struct, pglist);
      
      entry_ptr = malloc(sizeof(struct procgroup_struct));
      if (entry_ptr == NULL)
        return -1;
      
      entry_ptr->pid = procgroup1->pid;
      entry_ptr->gid = procgroup1->gid;
      list_add_tail(&entry_ptr->pglist, list2);
    }
  }
  else {
    list_for_each(tmp_head1, list2) {
      procgroup2 = list_entry(tmp_head1, struct procgroup_struct, pglist);
      
      if (procgroup2->gid >= gid)
        break;
    }
    list_for_each(tmp_head2, list1) {
      procgroup1 = list_entry(tmp_head2, struct procgroup_struct, pglist);
      
      entry_ptr = malloc(sizeof(struct procgroup_struct));
      if (entry_ptr == NULL)
        return -1;
      
      entry_ptr->pid = procgroup1->pid;
      entry_ptr->gid = procgroup1->gid;
      list_add_tail(&entry_ptr->pglist, tmp_head1);
    }
  }
  return 0;
}


/* merge a nearly received message */
extern void remote_merge(char *msg, int msg_len,
  struct sockaddr_in *raddr,
  struct timeval *arrival_ts) {
  char *ptr_head              = msg ;
  char *ptr_local_servers     = NULL ;
  char *ptr_remote_servers    = NULL ;
  
  unsigned int next_sendint ;
  int retval ;
  
  struct host_struct *rhost   = NULL ;
  
  struct timeval sending_ts ;
  unsigned int   seq          = 0 ;
  struct timeval thought_local_epoch ;
  struct timeval remote_epoch ;
  
  unsigned int   remote_servers_list_seq  = 0 ;
  unsigned int   remote_groups_list_seq   = 0 ;
  unsigned int   remote_servers_list_len  = 0 ;
  unsigned int   remote_servers_proc_count= 0 ;
  unsigned int   remote_groups_count      = 0 ;
  unsigned int   remote_sendint           = 0 ;
  unsigned int   remotevars_count         = 0 ;
  
  unsigned int   remote_clients_list_seq  = 0 ;
  unsigned int   local_servers_list_len   = 0 ;
  unsigned int   local_servers_proc_count = 0 ;
  unsigned int	 local_needed_sendint     = 0 ;
  
  unsigned int   eta_rcvd = 0 ;
  
  struct list_head remote_all_groups_procs_list ;
  struct list_head remote_group_procs_list ;
  struct list_head remote_servers_proc_list ;
  struct list_head local_servers_proc_list ;
  struct list_head *tmp_head;
  struct uint_struct *group;
  
  struct timeval local_largest_group_ts_rcvd,
  group_ts;
  
  
  int i ;
  char *ptr = NULL ;
  unsigned int gid ;
  int procs_count ;
  
  int new_remote_groups  = 0 ;
  int new_remote_clients = 0 ;
  int new_remote_servers = 0 ;
  int new_needed_sendint = 0 ;
  int send_back_eta      = 0 ;
  
#ifndef SYNCH_CLOCKS
  struct timeval delta_time ;
  int delta ;
#endif
  
  INIT_LIST_HEAD(&remote_servers_proc_list) ;
  INIT_LIST_HEAD(&remote_all_groups_procs_list) ;
  INIT_LIST_HEAD(&remote_group_procs_list) ;
  INIT_LIST_HEAD(&local_servers_proc_list) ;
  
  ptr_head = msg ;
  ptr_remote_servers =
  msg_parse_rep_head(ptr_head, &sending_ts, &seq,
    &remote_epoch,
    &thought_local_epoch,
    
    &remote_servers_list_seq,
    &remote_groups_list_seq,
    &remote_servers_list_len,
    &remote_servers_proc_count,
    &remote_groups_count,
    &remote_sendint,
    &remotevars_count,
    
    &remote_clients_list_seq,
    &local_servers_list_len,
    &local_servers_proc_count,
    &local_needed_sendint,
  &local_largest_group_ts_rcvd) ;
  
  
  retval = -EMSGSIZE ;
  if(ptr_remote_servers > msg + MAX_MSG_LEN) {
    goto out ;
  }
  
  retval = 0 ;
  if(timercmp(&thought_local_epoch, &local_epoch, <)) {
    /* message sent for the previos life of the FDD */
#ifdef OUTPUT
    fprintf(stdout, "REPORT Message seq = %u sent before localhost crashes\n",
    seq) ;
#endif
#ifdef LOG
    fprintf(flog, "REPORT Message seq = %u sent before localhost crashes\n",
    seq) ;
#endif
    goto out ;
  }
  
  if(timercmp(&thought_local_epoch, &local_epoch, >)) {
#ifdef OUTPUT
    fprintf(stdout, "INVALID REPORT Message received, seq = %u\n", seq) ;
#endif
#ifdef LOG
    fprintf(flog, "INVALID REPORT Message received, seq = %u\n", seq) ;
#endif
  }
  
  rhost = locate_create_host(raddr, &remote_epoch, seq - 1, arrival_ts) ;
  retval = -ENOMEM ;
  if(NULL == rhost)
    goto out ;
  
  
  retval = 0 ;
  /* received a message before having computed the initial value of
   the standard deviation: wrong! */
  if(rhost->stats.local_initial_finished != FINISHED_YES) {
    goto out ;
  }
  /* a message received from the remote before he told me that
  he finished the INITIAL. He couldn't send this message if he
  shouldn't have the INITIAL finished. So I declare the INITIAL
   finished for the remote */
  if(rhost->stats.remote_initial_finished != FINISHED_YES) {
    remove_initial_ed_event(rhost) ;
    stop_initial(rhost, arrival_ts) ;
  }
  
#ifndef SYNCH_CLOCKS
  if(timercmp(arrival_ts, &rhost->stats.expected_arrival_ts, >)) {
    timersub(arrival_ts, &rhost->stats.expected_arrival_ts, &delta_time) ;
    delta = (int)timer2unit(&delta_time) ;
  }
  else {
    timersub(&rhost->stats.expected_arrival_ts, arrival_ts, &delta_time) ;
    delta = -(int)timer2unit(&delta_time) ;
  }
#endif
  
#if defined( OUTPUT ) && !defined (SYNCH_CLOCKS)
  fprintf(stdout, "REMOTE_MERGE seq=%u, TS=%ld.%ld, ETS=%ld.%ld, "
    "delta=%i, actual_sendint=%u, "
    "group_count=%i\n", seq, arrival_ts->tv_sec,
    arrival_ts->tv_usec,
    rhost->stats.expected_arrival_ts.tv_sec,
    rhost->stats.expected_arrival_ts.tv_usec,
    delta,
  remote_sendint, remote_groups_count) ;
  fprintf(stdout, "prev_seq=%u, seq=%u in remote_merge\n",
  rhost->stats.last_seq, seq) ;
#endif
#if defined( LOG ) && !defined (SYNCH_CLOCKS)
  fprintf(flog, "REMOTE_MERGE seq=%u, TS=%ld.%ld, ETS=%ld.%ld, "
    "delta=%i, actual_sendint=%u, "
    "group_count=%i\n", seq, arrival_ts->tv_sec,
    arrival_ts->tv_usec,
    rhost->stats.expected_arrival_ts.tv_sec,
    rhost->stats.expected_arrival_ts.tv_usec,
    delta,
  remote_sendint, remote_groups_count) ;
  fprintf(flog, "prev_seq=%u, seq=%u in remote_merge\n",
  rhost->stats.last_seq, seq) ;
#endif
  
  retval = 0 ;
  if( timercmp(&rhost->remote_epoch, &remote_epoch, >)
    || greater_than(rhost->stats.last_seq, seq) ) {
    /* message sent before the host's crash or out of order message */
    
#ifdef OUTPUT
    fprintf(stdout, "REPORT_OUT_OF_ORDER: prev_seq=%u, seq=%u\n",
    rhost->stats.last_seq, seq) ;
#endif
#ifdef LOG
    fprintf(flog, "REPORT_OUT_OF_ORDER: prev_seq=%u, seq=%u\n",
    rhost->stats.last_seq, seq) ;
#endif
    /* compute the new statistics */
    stats_new_sample(rhost, seq, &sending_ts, arrival_ts, remote_sendint ) ;
    goto out;
  }
  
  /* parse the list of requested local servers */
  if(rhost->remote_clients_list_seq != remote_clients_list_seq) {
    rhost->local_servers_list_seq++ ;
    new_remote_clients = 1 ;
    ptr_local_servers = ptr_remote_servers + remote_servers_list_len ;
    retval = -EMSGSIZE ;
    if(ptr_local_servers > msg + MAX_MSG_LEN)
      goto out ;
    
    parse_proc_list(msg, ptr_local_servers, local_servers_proc_count,
    &local_servers_proc_list, &retval) ;
    
    if( retval < 0 )
      goto out_free_proc_list ;
  }
  
  
  /* parse the list of remote servers */
  if( rhost->last_local_clients_list_seq == rhost->local_clients_list_seq &&
    rhost->remote_servers_list_seq == remote_servers_list_seq) {
  }
  else {
    new_remote_servers = 1 ;
    ptr = parse_proc_list(msg, ptr_remote_servers, remote_servers_proc_count,
    &remote_servers_proc_list, &retval) ;
    if(retval < 0)
      goto out_free_proc_list ;
  }
  
  ptr = ptr_remote_servers + remote_servers_proc_count * sizeof(int) ;
  
  /* Added for Omega */
  /* If rhost has sent us a new group ts that is now greater or equal
  to the ts of one of the jointly groups, we increment local_groups_list_seq,
  so that rhost sees the process in that group in our next report. Remember that a host
  analyses the groups only if the report msg contains a greater
   remote_groups_list_seq! (See below) */
  list_for_each(tmp_head, &rhost->jointly_groups_head) {
    group = list_entry(tmp_head, struct uint_struct, uint_list);
    if (get_group_ts(group->val, &group_ts) < 0)
      fprintf(stderr, "fdd: Error in remote_merge, impossible to get ts of group: %u\n", group->val);
    
    /* If group ts of group group->val was greater than rhost->local_largest_group_ts_rcvd
     and now they are equal, we increment local_groups_list_seq. */
    if (timercmp(&rhost->local_largest_group_ts_rcvd, &group_ts, <) &&
      (timercmp(&group_ts, &local_largest_group_ts_rcvd, ==) ||
      timercmp(&group_ts, &local_largest_group_ts_rcvd, <))) {
      local_groups_list_seq++;
      break;
    }
  }
  
  memcpy(&rhost->local_largest_group_ts_rcvd, &local_largest_group_ts_rcvd,
  sizeof(struct timeval));
  
  
  /* parse the groups */
  if(
#ifndef SYNCH_CLOCKS
    1 ||
#endif
    !greater_than(rhost->remote_groups_list_seq, remote_groups_list_seq)) {
    struct uint_struct *entry_ptr ;
    int entry_exists ;
    
    new_remote_groups = 1 ;
    
    list_free(&rhost->list_remote_procs_in_groups_to_calc_eta,
    struct procgroup_struct, pglist);
    
    
    for(i = 0 ; i < remote_groups_count ; i++) {
      ptr = msg_parse_rep_ghead(ptr, &gid, &group_ts, &eta_rcvd, &procs_count) ;
      
      /* keep the largest group ts of this host */
      if (timercmp(&group_ts, &rhost->remote_largest_group_ts_rcvd, >))
        memcpy(&rhost->remote_largest_group_ts_rcvd, &group_ts, sizeof(struct timeval));
      
      entry_ptr = NULL ;
      retval = -ENOMEM ;
      list_insert_ordered(gid, val, &rhost->remote_all_groups_head,
      struct uint_struct, uint_list, <) ;
      if(entry_ptr == NULL)
        goto out_free_proc_list ;
      
      retval = sched_suspect_remote_group(rhost, entry_ptr, arrival_ts) ;
      if(retval < 0)
        goto out_free_proc_list ;
      
      ptr = parse_group_procs_list(msg, ptr, raddr, gid, procs_count,
      &remote_group_procs_list, eta_rcvd, &retval, arrival_ts) ;
      
      if (eta_rcvd) {
        if (add_proc_group_list2list(gid, &remote_group_procs_list,
          &remote_all_groups_procs_list) < 0) {
          fprintf(stderr, "fdd: Error in remote_merge while merging current proc_group");
          fprintf(stderr, "list with the one received\n");
        }
      }
      /* If we received at least one group with eta_rcvd = false it means
       that we must send a report containing the eta. */
      else
        send_back_eta = 1;
      
      list_splice(&remote_group_procs_list, &rhost->list_remote_procs_in_groups_to_calc_eta) ;
      
      INIT_LIST_HEAD(&remote_group_procs_list) ;
    }
    retval = build_jointly_groups_list(rhost,
    &rhost->remote_all_groups_head) ;
    if(retval < 0)
      goto out_free_proc_list ;
  }
  else {
    new_remote_groups = 0 ;
  }
  
  if(remote_sendint != rhost->remote_actual_sendint) {
    rhost->remote_actual_sendint = remote_sendint ;
  }
  
  rhost->stats.last_seq = seq ;
  stats_new_sample(rhost, seq, &sending_ts, arrival_ts, remote_sendint ) ;
  
  if( new_remote_servers )
  retval = build_trust_lists(rhost, &remote_servers_proc_list,
  &sending_ts, arrival_ts) ;
  else
  retval = build_trust_lists(rhost, &rhost->remote_servers_proc_head,
  &sending_ts, arrival_ts) ;
  if(retval < 0)
    goto out_free_trust_lists ;
  
  if( new_remote_groups )
    retval =
  build_all_procs_trust_lists_all_groups(rhost,
    &remote_all_groups_procs_list,
  &sending_ts, arrival_ts) ;
  else
    retval =
  build_all_procs_trust_lists_all_groups(rhost,
    &rhost->remote_all_groups_procs_head,
  &sending_ts, arrival_ts) ;
  if(retval < 0)
    goto out_free_trust_lists;
  
  
  ptr = ptr_remote_servers + remote_servers_list_len + local_servers_list_len;
  
  /* Added for Omega*/
  /* Parse the remotevars list received and insert them in the remotevars list */
  parse_and_insert_remotevars_list(msg, ptr, remotevars_count, raddr, &retval);
  
  if (retval < 0)
    goto out;
  
  if (remotevars_count != 0) {
    if (updateGlobalLeaders(arrival_ts) < 0)
      fprintf(stderr, "fdd Upon received alive: Error while updating global leaders\n");
  }
  
  
  /* we can't fail anymore, can commit changes by this message */
  /* switch list heads so that the old one will be freed further down */
  if( new_remote_servers )
    list_swap(&rhost->remote_servers_proc_head, &remote_servers_proc_list) ;
  
  if( new_remote_groups )
  list_swap(&rhost->remote_all_groups_procs_head,
  &remote_all_groups_procs_list) ;
  
  if( new_remote_clients )
    list_swap(&rhost->local_servers_proc_head, &local_servers_proc_list) ;
  
  memcpy(&rhost->sending_ts, &sending_ts, sizeof(sending_ts)) ;
  rhost->stats.last_seq = seq ;
  
  rhost->remote_servers_list_seq = remote_servers_list_seq ;
  rhost->last_local_clients_list_seq = rhost->local_clients_list_seq ;
  rhost->remote_clients_list_seq = remote_clients_list_seq ;
  
  if(new_remote_groups)
    rhost->remote_groups_list_seq = remote_groups_list_seq ;
  
  //  rhost->remote_actual_sendint = remote_sendint ;
  
  rhost->local_needed_sendint = local_needed_sendint;
  recalc_needed_sendint(rhost, arrival_ts) ;
  
  
  /* Modified for Omega */
  /* Error margin introduced for the remote_sendint */
  if ((rhost->remote_needed_sendint < remote_sendint) ||
    (rhost->remote_needed_sendint > (1.25 * remote_sendint))) {
    new_needed_sendint = 1;
  }
  
  
  if (send_back_eta) {
    next_sendint = 0;
  }
  else {
#ifdef SYNCH_CLOCKS
    if( (new_needed_sendint || new_remote_groups ) ) {
      next_sendint = 0 ;
    }
    else
#endif
    next_sendint = rhost->local_needed_sendint ;
  }
  
  local_sched_report_sooner(next_sendint, arrival_ts, rhost) ;
  
  /*  if( rhost->stats.local_initial_finished == FINISHED_YES &&
   rhost->stats.remote_initial_finished ) */
  commit_trust_lists(rhost, arrival_ts) ;
  
  retval = 0 ;
  
  out_free_trust_lists:
  all_trust_lists_free() ;
  
  out_free_proc_list:
  list_free(&remote_group_procs_list, struct procgroup_struct, pglist) ;
  list_free(&remote_all_groups_procs_list, struct procgroup_struct, pglist) ;
  list_free(&remote_servers_proc_list, struct uint_struct, uint_list) ;
  list_free(&local_servers_proc_list, struct uint_struct, uint_list) ;
  //  list_free(&remote_groups, struct uint_struct, uint_list) ;
  
  out:
  if (retval < 0) {
#ifdef OUTPUT
    fprintf(stderr, "fdd: remote_merge failed\n") ;
#endif
#ifdef LOG
    fprintf(flog, "fdd: remote_merge failed\n") ;
#endif
  }
}

extern void show_host(struct list_head *mng_hosts, int n) {
  struct host_struct *host = NULL ;
  
  struct list_head *tmp_mng = NULL ;
  struct mng_host_struct *mng = NULL ;
  int i = 1 ;
  
  list_for_each(tmp_mng, mng_hosts) {
    mng = list_entry(tmp_mng, struct mng_host_struct, mng_host_list) ;
    if( n != i ) {
      i++ ;
      continue ;
    }
    
    host = locate_host(&mng->addr) ;
    if( NULL == host ) {
      printf("Sorry, the specified host(%u.%u.%u.%u) does not longer exist\n",
      NIPQUAD(&mng->addr)) ;
      goto out ;
    }
    
    printf("System parameters: pl=%f, ED=%fms, VD=%fms, "
      "ExpArriv=%ld.%ld, l=%u\n",
      host->stats.est.pl, host->stats.est.e_d, host->stats.est.v_d,
      host->stats.expected_arrival_ts.tv_sec,
      host->stats.expected_arrival_ts.tv_usec,
    host->stats.last_seq) ;
    
    {
      struct uint_struct *group = NULL ;
      struct list_head *tmp_jointly = NULL ;
      printf("Host %u.%u.%u.%u has the following jointly groups:",
      NIPQUAD(&host->addr)) ;
      list_for_each(tmp_jointly, &host->jointly_groups_head) {
        group = list_entry(tmp_jointly, struct uint_struct, uint_list) ;
        printf(" %u", group->val) ;
      }
      printf("\n") ;
      
      printf("Host %u.%u.%u.%u has the following groups:",
      NIPQUAD(&host->addr)) ;
      list_for_each(tmp_jointly, &host->remote_all_groups_head) {
        group = list_entry(tmp_jointly, struct uint_struct, uint_list) ;
        printf(" %u", group->val) ;
      }
      printf("\n") ;
    }
    
    
    {
      struct list_head *tmp_pg = NULL ;
      struct procgroup_struct *procgroup = NULL ;
      
      printf("Host %u.%u.%u.%u has the following group-processes:",
      NIPQUAD(&host->addr)) ;
      list_for_each(tmp_pg, &host->remote_all_groups_procs_head) {
        procgroup = list_entry(tmp_pg, struct procgroup_struct, pglist) ;
        printf(" (pid=%u, gid=%u), ", procgroup->pid, procgroup->gid) ;
      }
      printf("\n") ;
    }
    
    {
      struct list_head *tmp_proc = NULL ;
      struct uint_struct *proc_pid = NULL ;
      
      struct localproc_struct *lproc = NULL ;
      
      struct list_head *tmp_pqos = NULL ;
      struct procqos_struct *pqos = NULL ;
      
      printf("Local clients for the remote host:\n") ;
      list_for_each(tmp_proc, &host->local_clients_proc_head) {
        proc_pid = list_entry(tmp_proc, struct uint_struct, uint_list) ;
        lproc = get_local_proc(proc_pid->val) ;
        
        list_for_each(tmp_pqos, &lproc->pqlist_head) {
          pqos = list_entry(tmp_pqos, struct procqos_struct, pqlist) ;
          if( sockaddr_eq(&pqos->addr, &host->addr) )
          printf("Local process fdd_pid=%u is client for fdd_pid=%u on "
            "%u.%u.%u.%u, with the QoS=(TdU=%ums,TmU=%ums,TmrL=%us)\n",
            proc_pid->val, pqos->pid, NIPQUAD(&host->addr),
          pqos->qos.TdU, pqos->qos.TmU, pqos->qos.TmrL) ;
        }
      }
    }
    
    {
      struct list_head *tmp_proc = NULL ;
      struct uint_struct *proc_pid = NULL ;
      
      printf("Local servers for the remote host:") ;
      list_for_each(tmp_proc, &host->local_servers_proc_head) {
        proc_pid = list_entry(tmp_proc, struct uint_struct, uint_list) ;
        printf(" %u", proc_pid->val) ;
      }
      printf("\n") ;
    }
    goto out ;
  }
  
  out:
  return ;
}

extern void build_mng_host_list(struct list_head *mng_hosts) {
  
  struct list_head *tmp_host = NULL ;
  struct host_struct *host = NULL ;
  struct mng_host_struct *mng = NULL ;
  int i = 1 ;
  
  list_free(mng_hosts, struct mng_host_struct, mng_host_list) ;
  
  list_for_each(tmp_host, &remote_host_list_head) {
    host = list_entry(tmp_host, struct host_struct, remote_host_list) ;
    mng = malloc(sizeof(*mng)) ;
    if(NULL == mng) {
#ifdef OUTPUT
      fprintf(stdout, "build_mng_host_list:%s\n", strerror(errno)) ;
#endif
#ifdef LOG
      fprintf(flog, "build_mng_host_list:%s\n", strerror(errno)) ;
#endif
      goto out ;
    }
    memcpy(&mng->addr, &host->addr, sizeof(mng->addr)) ;
    list_add(&mng->mng_host_list, mng_hosts) ;
  }
  
  list_for_each(tmp_host, mng_hosts) {
    mng = list_entry(tmp_host, struct mng_host_struct, mng_host_list) ;
    printf("%i. Host: %u.%u.%u.%u\n", i, NIPQUAD(&mng->addr)) ;
    i++ ;
  }
  
  out:
  return ;
}

extern void suspect_remote_group(struct host_struct *host,
  struct uint_struct *group) {
  
  list_del(&group->uint_list) ;
  free(group) ;
  build_jointly_groups_list(host, &host->remote_all_groups_head) ;
  
}



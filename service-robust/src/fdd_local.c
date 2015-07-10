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


/* fdd_local.h */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include "fdd.h"
#include "variables_exchange.h"
#include "misc.h"
#include "omega.h"
#include <signal.h>


struct list_head local_procs_list_head ; /* list of local processes */
struct list_head local_groups_list_head ; /* list of local groups */


/* Added for Omega */
/* List containing for all the group g of all the local processes p if p
 is visible in g */
struct list_head visibility_list_head;

/* Added for Omega */
/* List that contains for all group g, a timestamp that correspond to the
the time at which the first process joined g or when a process
 restarted sending alives in group g. */
struct list_head group_join_ts_head;

struct timeval local_epoch ;

unsigned int local_groups_list_seq ;
unsigned int local_groups_multicast_list_seq ;

static struct timeval last_report_ts, next_report_ts ;

/* imported stuff */
#ifdef LOG
extern FILE *flog ;
#endif


static struct localproc_struct *find_lproc(int pid, int *found) {
  
  struct list_head *tmp = NULL;
  struct localproc_struct *lproc = NULL;
  
  *found = 0;
  
  list_for_each(tmp, &local_procs_list_head) {
    lproc = list_entry(tmp, struct localproc_struct, local_procs_list);
    
    if(lproc->pid == pid) {
      *found = 1;
      break;
    }
  }
  return lproc;
}



/* send TRUST notification to the local proces lproc for all other local
 processes which joined the group gid */
static int local_trust_group(struct localproc_struct *lproc, unsigned int gid,
  struct timeval *now) {
  struct list_head *tmp ;
  struct trust_struct trust ;
  struct groupqos_struct *gqos ;
  struct localproc_struct *entry ;
  
  int retval ;
  
  retval = -ENOMEM ;
  trust.gid = malloc(sizeof(*trust.gid)) ;
  if(NULL == trust.gid )
    goto out ;
  trust.gid->val = gid ;
  trust.host = NULL ;
  
  /* for all local processes */
  list_for_each(tmp, &local_procs_list_head) {
    entry = list_entry(tmp, struct localproc_struct, local_procs_list) ;
    /* skip if it is the same local process as the one given in arguments */
    if(entry->pid == lproc->pid)
      continue ;
    gqos = locate_gqos(entry, gid) ;
    /* skip if the local process doesn't belong to any group or
     if no defined group qos or if interruption not wanted */
    if( NULL == gqos || NULL == gqos->qos
      || 0 == gqos->qos->TdU ||
    (INTERRUPT_ANY_CHANGE != (trust.int_type = gqos->int_type)) )
    continue ;
    trust.pid = entry->pid ;
    /* send notification */
    local_change_notify(lproc, TRUST_NOTIF, &trust, now) ;
  }
  
  free(trust.gid) ;
  retval = 0 ;
  out:
  return retval ;
}

/* send to the whole group gid, the notification not_type
 concerning the local proces pid */
static int notify_local_group(unsigned int pid, unsigned int gid, u_int not_type,
  struct timeval *now) {
  
  struct groupqos_struct *gqos ;
  struct localproc_struct *lproc ;
  struct trust_struct trust ;
  struct list_head *tmp ;
  int retval ;
  
  trust.pid = pid ;
  trust.host = NULL ;
  
  retval = -ENOMEM ;
  trust.gid = malloc(sizeof(*trust.gid)) ;
  if(trust.gid == NULL)
    goto out ;
  trust.gid->val = gid ;
  
  /* for each local process */
  list_for_each(tmp, &local_procs_list_head) {
    lproc = list_entry(tmp, struct localproc_struct, local_procs_list) ;
    if( lproc->pid == pid ) /* skip if the same as the one given */
      continue ;
    gqos = locate_gqos(lproc, gid) ;
    if(NULL == gqos || NULL == gqos->qos
    || (INTERRUPT_ANY_CHANGE != (trust.int_type = gqos->int_type) ) )
    /* skip if doesn't belong to
    any group or if no group qos specified
     or if interruption not wanted */
    continue ;
    /* send him the notification */
    local_change_notify(lproc, not_type, &trust, now) ;
  }
  
  free(trust.gid) ;
  retval = 0 ;
  out:
  return retval ;
}

/* send a notification to each local process which monitors the process pid in
 * the addr machine */
static void notify_local(unsigned int pid,
  struct sockaddr_in *addr,
  u_int not_type, struct timeval *now) {
  struct list_head *tmp_lproc       = NULL ;
  /*    struct list_head *tmp_pqos        = NULL ; */
  struct localproc_struct *lproc    = NULL ;
  struct procqos_struct *pqos       = NULL ;
  struct trust_struct trust ;
  
  trust.pid = pid ;
  trust.host = NULL ;
  trust.gid = NULL ;
  
  /* trust.fresh is not used */
  list_for_each(tmp_lproc, &local_procs_list_head) {
    lproc = list_entry(tmp_lproc, struct localproc_struct, local_procs_list) ;
    
    pqos = locate_pqos(lproc, pid, addr) ;
    if(NULL != pqos &&
      INTERRUPT_ANY_CHANGE == (trust.int_type = pqos->int_type)) {
      local_change_notify(lproc, not_type, &trust, now);
      /*      list_for_each(tmp_pqos, &lproc->pqlist_head) { */
      /*        pqos = list_entry(tmp_pqos, struct procqos_struct, pqlist) ;  */
      
      /*        if(pid == pqos->pid && sockaddr_eq(&pqos->addr, addr) */
      /*  	 && INTERRUPT_ANY_CHANGE == pqos->int_type ) */
      /*  	local_change_notify(lproc, not_type, &trust, now); */
      
    }
  }
}

/* remove all the reqest for monitoring from a local process to any host */
static void unreg_lproc_from_related_hosts(unsigned int pid,
  struct timeval *unreg_ts) {
  struct list_head *tmp_host     = NULL ;
  struct host_struct *host       = NULL ;
  int found ;
  int status_modified_host = 0 ; /* 0 - host not touched
  1 - host modified => need to send report
   */
  /* chech all the hosts */
  list_for_each(tmp_host, &remote_host_list_head) {
    host = list_entry(tmp_host, struct host_struct, remote_host_list) ;
    
    /* check if this process was giving service to this host */
    if(is_in_ordered_list(pid, &host->local_servers_proc_head)) {
      host->local_servers_list_seq++ ;
      status_modified_host = 1 ;
    }
    
    /* check if this process was requesting service from this host */
    found = 0 ;
    list_remove_ordered(pid, val, &host->local_clients_proc_head,
    struct uint_struct, uint_list, <) ;
    if(found) {
      host->local_clients_list_seq++ ;
      recalc_needed_sendint(host, unreg_ts) ;
      status_modified_host = 1 ;
    }
    
    if(status_modified_host) {
      local_sched_report_sooner(host->local_needed_sendint, unreg_ts, host) ;
      status_modified_host = 0 ;
    }
  }
}

/* check if the given group is in the list of ordered group IDs */
static int is_in_ordered_gqlist(unsigned gid, struct list_head *gqolist) {
  
  struct list_head *tmp_gqos = NULL ;
  struct groupqos_struct *gqos = NULL ;
  
  list_for_each(tmp_gqos, gqolist) {
    gqos = list_entry(tmp_gqos, struct groupqos_struct, gqlist) ;
    if(gqos->gid < gid)
      continue ;
    if(gqos->gid == gid)
      return 1 ;
    break ;
  }
  return 0 ;
}


/* Added for Omega */
/* When a process joins a new group or when a process restarts sending
alives for group gid, this procedure is called. The group timestamp
 is used in the handshake that occurs at the beginning of a monitoring.*/
static int add_group_ts(unsigned int gid, struct timeval *now) {
  
  int entry_exists;
  struct group_ts_struct *entry_ptr = NULL;
  
  list_insert_ordered(gid, gid, &group_join_ts_head, struct group_ts_struct,
  group_ts_list, <);
  
  if (entry_ptr == NULL)
    return -1;
  
  memcpy(&entry_ptr->ts, now, sizeof(struct timeval));
  return 0;
}


/* Added for Omega */
/* Adds or updates the group ts. */
static int add_update_group_ts(unsigned int gid, struct timeval *now) {
  
  int entry_exists;
  struct group_ts_struct *entry_ptr = NULL;
  
  list_insert_ordered(gid, gid, &group_join_ts_head, struct group_ts_struct,
  group_ts_list, <);
  
  if (entry_ptr == NULL)
    return -1;
  
  memcpy(&entry_ptr->ts, now, sizeof(struct timeval));
  return 0;
  
}

inline int get_group_ts(unsigned int gid, struct timeval *group_ts) {
  
  struct list_head *tmp_head;
  struct group_ts_struct *group_timestamp;
  
  list_for_each(tmp_head, &group_join_ts_head) {
    group_timestamp = list_entry(tmp_head, struct group_ts_struct, group_ts_list);
    if (group_timestamp->gid < gid)
      continue;
    else if (group_timestamp->gid == gid) {
      memcpy(group_ts, &group_timestamp->ts, sizeof(struct timeval));
      return 0;
    }
    else
      return -1;
  }
  return -1;
}


int get_largest_jointly_group_ts(struct host_struct *rhost, struct timeval *largest_jointly_group_ts) {
  
  struct list_head *tmp_head1;
  struct uint_struct *group;
  struct timeval group_ts;
  
  timerclear(largest_jointly_group_ts);
  
  list_for_each(tmp_head1, &rhost->jointly_groups_head) {
    group = list_entry(tmp_head1, struct uint_struct, uint_list);
    if (get_group_ts(group->val, &group_ts) < 0)
      return -1;
    
    if (timercmp(&group_ts, largest_jointly_group_ts, >))
      memcpy(largest_jointly_group_ts, &group_ts, sizeof(struct timeval));
  }
  return 0;
}


/* Added for Omega */
/* When a process leaves a group that becomes empty this procedure is called. */
static int remove_group_ts(unsigned int gid) {
  
  int found = 0;
  
  list_remove_ordered(gid, gid, &group_join_ts_head, struct group_ts_struct,
  group_ts_list, <);
  
  if (found)
    return 0;
  else
    return -1;
}




/* check if there is any local process who joined the group gid */
static int exists_group(unsigned int gid) {
  struct list_head *tmp_lproc ;
  struct localproc_struct *lproc ;
  
  list_for_each(tmp_lproc, &local_procs_list_head) {
    lproc = list_entry(tmp_lproc, struct localproc_struct, local_procs_list) ;
    
    if(is_in_ordered_gqlist(gid, &lproc->gqlist_head))
      return 1 ;
  }
  return 0 ;
}

/* a processes leaves a group */
static int proc_quit_group(struct localproc_struct *lproc,
  unsigned int gid, struct timeval *now) {
  
  int retval ;
  int pid = lproc->pid;
  
  
  /* Added for Omega */
  /* Removes the group gid from the visibility list */
  visibility_list_group_free(pid, gid);
  
  if(!exists_group(gid)) {
    int found ;
    list_remove_ordered(gid, val, &local_groups_list_head, struct uint_struct,
    uint_list, <) ;
    
    if (remove_group_ts(gid) < 0)
    fprintf(stderr, "fdd: Error in proc_quit_group, impossible to remove"
    "timestamp of group: %u\n", gid);
    
    /* Added for Omega */
    /* When there are no more processes which are in group gid,
    we can broadcast a hello message to tell the others that group gid
     is not present anymore on this machine. */
    sched_hello_multicast_now();
  }
  
  local_groups_list_seq++ ;
  local_groups_multicast_list_seq++ ;
  
  {
    struct list_head *tmp ;
    struct host_struct *host ;
    
    list_for_each(tmp, &remote_host_list_head) {
      host = list_entry(tmp, struct host_struct, remote_host_list) ;
      
      if(is_in_ordered_list(gid, &host->jointly_groups_head)) {
        build_jointly_groups_list(host, &host->remote_all_groups_head) ;
        recalc_needed_sendint(host, now) ;
        local_sched_report_sooner(host->local_needed_sendint, now, host) ;
      }
    }
  }
  
  
  /* Added for Omega */
  /* Removes the localvars of group gid if no local process belong to that group
   anymore. We don't have to do that for the FDD group. */
  if ((gid != 0) && !exists_group(gid))
    remove_localvars(gid);
  
  retval = notify_local_group(lproc->pid, gid, CRASHED_NOTIF, now) ;
  return retval ;
}

/* a local process leaves all groups */
static void proc_quit_all_groups(struct localproc_struct *lproc, struct timeval *now) {
  
  struct list_head *tmp_gqos = NULL ;
  struct groupqos_struct *gqos = NULL ;
  
  list_for_each(tmp_gqos, &lproc->gqlist_head) {
    gqos = list_entry(tmp_gqos, struct groupqos_struct, gqlist) ;
    tmp_gqos = tmp_gqos->prev ;
    
    list_del(&gqos->gqlist) ;
    proc_quit_group(lproc, gqos->gid, now) ;
    free_gqos(gqos) ;
  }
}

/* unregister a local process */
extern int fdd_local_unreg(unsigned int pid, struct timeval *unreg_ts) {
  
  struct localproc_struct *lproc = NULL;
  struct list_head *tmp_head1, *tmp_head2;
  struct uint_struct *proc;
  struct host_struct *host;
  struct uint_struct *group;
  struct timeval now;
  int alive_scheduled = 0;
  int found;
  
  
  lproc = find_lproc(pid, &found);
  if (!found)
    return -1;
  
  
  /* Added for Omega*/
  /* Removes the process and its group from the visibility_list_head variable */
  visibility_list_proc_free(pid);
  
  
  /* Added for Omega */
  /* When a process unregisters or crashes we send a report message
  to all the hosts that monitored this process either by mean of group
   or p2p monitoring. */
  /* Set time to 1st january 1970, the report msg will therefore be sent right away. */
  timerclear(&now);
  
  list_for_each(tmp_head1, &remote_host_list_head) {
    host = list_entry(tmp_head1, struct host_struct, remote_host_list);
    list_for_each(tmp_head2, &host->local_servers_proc_head) {
      proc = list_entry(tmp_head2, struct uint_struct, uint_list);
      if (proc->val == pid) {
        if ((host->stats.local_initial_finished == FINISHED_YES) &&
          (host->stats.remote_initial_finished == FINISHED_YES)) {
          sched_report_now(host, &now);
          alive_scheduled = 1;
          break;
        }
      }
    }
    
    if (!alive_scheduled) {
      list_for_each(tmp_head2, &host->jointly_groups_head) {
        group = list_entry(tmp_head2, struct uint_struct, uint_list);
        if (is_in_ordered_gqlist(group->val, &lproc->gqlist_head)) {
          if ((host->stats.local_initial_finished == FINISHED_YES) &&
            (host->stats.remote_initial_finished == FINISHED_YES)) {
            sched_report_now(host, &now);
            break;
          }
        }
      }
    }
  }
  
  
  list_del(&lproc->local_procs_list);
  
  /* remove all the trusts of this process in any host */
  local_untrust_host(lproc, NULL, NULL);
  /* notify all local processes that it crashed */
  notify_local(lproc->pid, &fdd_local_addr, CRASHED_NOTIF, unreg_ts);
  
  /* remove the requested remote servers from all the hosts */
  unreg_lproc_from_related_hosts(pid, unreg_ts) ;
  
  /* leaves all groups */
  proc_quit_all_groups(lproc, unreg_ts) ;
  
  list_free(&lproc->pqlist_head, struct procqos_struct, pqlist) ;
  trust_list_free(&lproc->tmp_tlist_head) ;
  trust_list_free(&lproc->trust_list_head) ;
  free(lproc) ;
  return 0;
}

/* send result message type to a local process */
/*static int send_result(struct localproc_struct *lproc, int result, int extra,
struct timeval *ts) {
char msg[FIFO_MSG_LEN] ;
msg_build_res(msg, result, extra, ts) ;
return write_msg(lproc->qry_fd, msg, FIFO_MSG_LEN) ;
 }*/

/* build the list of requested remote servers from all the host
 by adding the requested remote servers by a local process */
static void reg_lproc_in_related_hosts(unsigned int pid,
  struct timeval *reg_ts) {
  
  struct list_head *tmp_host     = NULL ;
  struct host_struct *host       = NULL ;
  
  /* chech all hosts */
  list_for_each(tmp_host, &remote_host_list_head) {
    host = list_entry(tmp_host, struct host_struct, remote_host_list) ;
    
    /* check if this process has to give service to this host */
    
    if(is_in_ordered_list(pid, &host->local_servers_proc_head)) {
      host->local_servers_list_seq++ ;
      // local_sched_report_sooner(0, reg_ts, host) ;
    }
  }
}

/* check if the local process is alive */
extern int isalive(unsigned int pid) {
  
  struct list_head *tmp_lproc ;
  struct localproc_struct *lproc ;
  
  list_for_each(tmp_lproc, &local_procs_list_head) {
    lproc = list_entry(tmp_lproc, struct localproc_struct, local_procs_list) ;
    if(lproc->pid == pid)
      return 1;
    if(lproc->pid > pid)
      break ;
  }
  return 0 ;
}

/* stop monitoring a remote process */
/*static void do_stop_monitor_proc(struct localproc_struct *lproc, char *msg,
struct timeval *now) {

unsigned int pid ;
struct sockaddr_in addr ;
int retval ;
struct list_head *tmp ;
struct procqos_struct *pqos ;
struct host_struct *host ;

msg_parse_stop_monproc(msg, &pid, &addr) ;

retval = -EINVAL ;
list_for_each(tmp, &lproc->pqlist_head) {
pqos = list_entry(tmp, struct procqos_struct, pqlist) ;
if(pqos->pid == pid && sockaddr_eq(&pqos->addr, &addr)) {
  list_del(&pqos->pqlist) ;
free(pqos) ;
retval = 0 ;
break ;
}
if(pqos->pid > pid)
  goto out ;
}

host = locate_host(&addr) ;
if(host)
  remote_replay_host(lproc, host, now);

out:
retval = send_result(lproc, retval, 0, now) ;
 }*/

/* start monitoring a remote process or edit the qos */
/*static void do_monitor_proc(struct localproc_struct *lproc, char *msg,
struct timeval *now) {
struct list_head *tmp     = NULL ;
struct sockaddr_in addr ;
struct procqos_struct *pq = NULL ;
struct host_struct *host  = NULL ;

unsigned int pid ;
unsigned int TdU, TmU, TmrL ;
int retval ;
int local_monitoring = 0 ;
u_int int_type ;

msg_parse_monproc(msg, &pid, &addr, &int_type, &TdU, &TmU, &TmrL) ;
#ifdef OUTPUT
fprintf(stdout, "MonProc(pid=%u,host=%u.%u.%u.%u)\n", pid, NIPQUAD(&addr)) ;
#endif
#ifdef LOG
fprintf(flog, "MonProc(pid=%u,host=%u.%u.%u.%u)\n", pid, NIPQUAD(&addr)) ;
#endif

// if exist already, remove the old one
list_for_each(tmp, &lproc->pqlist_head) {
pq = list_entry(tmp, struct procqos_struct, pqlist) ;
if(pq->pid == pid && sockaddr_eq(&pq->addr, &addr)) {
  tmp = tmp->prev ;
list_del(&pq->pqlist) ;
free(pq) ;
break ;
}
if(pq->pid > pid)
  break ;
}

// bogus qos convention
retval = 0 ;
if(0 == TdU)
  goto out ;

pq = malloc(sizeof(*pq));
retval = -ENOMEM;
if(NULL == pq) {
#ifdef OUTPUT
fprintf(stderr, "do_monitor_proc:%s\n", strerror(errno)) ;
#endif
#ifdef LOG
fprintf(flog, "do_monitor_proc:%s\n", strerror(errno)) ;
#endif
goto out;
}
addr.sin_port = htons(DEFAULT_PORT) ;
addr.sin_family = PF_INET ;

pq->pid = pid;
memcpy(&pq->addr, &addr, sizeof(addr));
pq->int_type = int_type ;
pq->qos.TdU = TdU;
pq->qos.TmU = TmU;
pq->qos.TmrL = TmrL;

list_add_tail(&pq->pqlist, &lproc->pqlist_head);
retval = 0 ;
if( sockaddr_eq(&addr, &fdd_local_addr) ) {
  local_monitoring = 1 ;
goto out ;
}

host = locate_create_host(&addr, NULL, 0, now) ;

if( NULL == host )
  goto out ;

host->addr.sin_port = htons(DEFAULT_PORT) ;
host->addr.sin_family = PF_INET ;

add_local_client(host, lproc) ;

host->local_clients_list_seq++ ;

if(host->stats.local_initial_finished == FINISHED_YES &&
  host->stats.remote_initial_finished == FINISHED_YES) {
recalc_needed_sendint(host, now) ;
local_sched_report_sooner(host->local_needed_sendint, now, host) ;
}

// rebuild the trust list for the local process
remote_replay_host(lproc, host, now);
retval = 0;

out:
retval = send_result(lproc, retval, 0, now);
if (retval < 0) {
#ifdef OUTPUT
fprintf(stderr, "fdd: send_result failed\n");
#endif
#ifdef LOG
fprintf(flog, "fdd: send_result failed\n");
#endif
}
if(local_monitoring)
  notify_local(pid, &addr, (isalive(pid)? TRUST_NOTIF : CRASHED_NOTIF), now) ;
else
  // Added for Omega
// send a report message directly
sched_report_now(host, now);
 }*/

/* checks if a process is member of the trust list of the local process */
extern int proc_trust_list_member(struct localproc_struct *lproc,
  unsigned int pid,
  unsigned int gid,
  unsigned int group_available,
  struct sockaddr_in *addr) {
  struct list_head *tmp ;
  struct trust_struct *trust ;
  int retval ;
  
  if(sockaddr_eq(addr, &fdd_local_addr))
    return isalive(pid) ;
  
  retval = 1 ;
  list_for_each(tmp, &lproc->trust_list_head) {
    trust = list_entry(tmp, struct trust_struct, trust_list) ;
    if(trust->pid == pid && sockaddr_eq(addr, &trust->host->addr)) {
      if(group_available) {
        if(trust->gid && trust->gid->val == gid )
          goto out ;
      }
      else {
        goto out ;
      }
    }
  }
  retval = 0 ;
  out:
  return retval ;
}

/* query a group for all the processes that are alive in this group */
/*static void do_query_group(struct localproc_struct *lproc, char *msg,
struct timeval *now) {

struct groupqos_struct *gqos ;
int pcount ;

struct list_head *tmp ;
struct trust_struct *trust ;
char msg2[FDD_QUERY_PROC_LEN] ;
struct localproc_struct *entry ;
int retval ;
unsigned int gid ;

msg_parse_qrygroup_head(msg, &gid) ;

pcount = 0 ;
retval = -EINVAL ;
gqos = locate_gqos(lproc, gid) ;
// if the local process doesn't belong to the given group or if doesn't mointor it
return an error
if(NULL == gqos || NULL == gqos->qos) {
  retval = send_result(lproc, retval, 0, now) ;
goto out ;
}

// count the number of REMOTE trusted processes in the given group
list_for_each(tmp, &local_procs_list_head) {
entry = list_entry(tmp, struct localproc_struct, local_procs_list) ;
if(entry->pid == lproc->pid)
  continue ;

if(is_in_ordered_gqlist(gqos->gid, &entry->gqlist_head))
  pcount++ ;
}
// add the LOCAL processes trusted in the given group
list_for_each(tmp, &lproc->trust_list_head) {
trust = list_entry(tmp, struct trust_struct, trust_list) ;
if(trust->gid && trust->gid->val == gid)
  pcount++ ;
}

// send the number of processes which will be sent
retval = 0 ;
retval = send_result(lproc, retval, pcount, now) ;
if (retval < 0)
  goto out ;

// send all trusted remote processes in the given group
list_for_each(tmp, &lproc->trust_list_head) {
trust = list_entry(tmp, struct trust_struct, trust_list) ;
if(NULL == trust->gid || gid != trust->gid->val)
  continue ;
msg_build_qrygroup_proc(msg2, trust->pid, trust->gid->val, 1, TRUST_NOTIF, &trust->host->addr) ;
retval = write_msg(lproc->qry_fd, msg2, FDD_QUERY_PROC_LEN) ;
if (retval < 0)
  goto out ;
}

// send all alive local processes in the given group
list_for_each(tmp, &local_procs_list_head) {
entry = list_entry(tmp, struct localproc_struct, local_procs_list) ;
if(entry->pid == lproc->pid)
  continue ;

if(is_in_ordered_gqlist(gqos->gid, &entry->gqlist_head)) {
  msg_build_qrygroup_proc(msg2, entry->pid, gqos->gid, 1, TRUST_NOTIF, &fdd_local_addr) ;
retval = write_msg(lproc->qry_fd, msg2, FDD_QUERY_PROC_LEN) ;
if(retval < 0)
  goto out ;
break ;
}
}

retval = 0 ;

out:
if (retval < 0)
  fprintf(stderr, "fdd: do_query_all failed\n");
 }*/

/* check for any alive process no matter if it is alive as Point to Point of group */
/*static void do_query_all(struct localproc_struct *lproc, char *msg,
struct timeval *now) {
int pcount ;
int retval ;
struct list_head *tmp ;
struct list_head *tmp_gqos ;
struct trust_struct *trust ;
char msg2[FDD_QUERY_PROC_LEN] ;
unsigned int is_trusted ;
struct procqos_struct *pqos ;
struct groupqos_struct *gqos ;
struct localproc_struct *entry ;
pcount = 0 ;

// count the number of monitored processes by the local process
list_for_each(tmp, &lproc->pqlist_head)
pcount++ ;

// count the number of trusted REMOTE processes as group monitoring
list_for_each(tmp, &lproc->trust_list_head) {
trust = list_entry(tmp, struct trust_struct, trust_list) ;
if(trust->gid)
  pcount++ ;
}

// count the number of trusted LOCAL processes as group monitoring
list_for_each(tmp, &local_procs_list_head) {
entry = list_entry(tmp, struct localproc_struct, local_procs_list) ;
if(entry->pid == lproc->pid)
  continue ;

list_for_each(tmp_gqos, &lproc->gqlist_head) {
gqos = list_entry(tmp_gqos, struct groupqos_struct, gqlist) ;
if(NULL == gqos->qos)
  continue ;
if(is_in_ordered_gqlist(gqos->gid, &entry->gqlist_head)) {
  pcount++ ;
break ;
}
}
}
// send the number of processes which states will be sent next
retval = send_result(lproc, 0, pcount, now) ;
if (retval < 0)
  goto out ;
// send the state of each Point to Point monitored process
list_for_each(tmp, &lproc->pqlist_head) {
pqos = list_entry(tmp, struct procqos_struct, pqlist) ;
if(proc_trust_list_member(lproc, pqos->pid, 0, 0, &pqos->addr))
  is_trusted = TRUST_NOTIF ;
else
  is_trusted = CRASHED_NOTIF ;

msg_build_qryall_proc(msg2, pqos->pid, 0, 0, is_trusted, &pqos->addr) ;
retval = write_msg(lproc->qry_fd, msg2, FDD_QUERY_PROC_LEN) ;
if(retval < 0)
  goto out ;
}
// send the state of each trusted remote processes belonging to a group
list_for_each(tmp, &lproc->trust_list_head) {
trust = list_entry(tmp, struct trust_struct, trust_list) ;
if(NULL == trust->gid)
  continue ;
msg_build_qryall_proc(msg2, trust->pid, trust->gid->val, 1, TRUST_NOTIF, &trust->host->addr) ;
retval = write_msg(lproc->qry_fd, msg2, FDD_QUERY_PROC_LEN) ;
if (retval < 0)
  goto out ;
}
// send the state of each trusted local process belonging to a group
list_for_each(tmp, &local_procs_list_head) {
entry = list_entry(tmp, struct localproc_struct, local_procs_list) ;
if(entry->pid == lproc->pid)
  continue ;
list_for_each(tmp_gqos, &lproc->gqlist_head) {
gqos = list_entry(tmp_gqos, struct groupqos_struct, gqlist) ;
if(NULL == gqos->qos)
  continue ;
if(is_in_ordered_gqlist(gqos->gid, &entry->gqlist_head)) {
  msg_build_qryall_proc(msg2, entry->pid, gqos->gid, 1, TRUST_NOTIF, &fdd_local_addr) ;
retval = write_msg(lproc->qry_fd, msg2, FDD_QUERY_PROC_LEN) ;
if(retval < 0)
  goto out ;
break ;
}
}
}

retval = 0 ;

out:
if (retval < 0)
  fprintf(stderr, "fdd: do_query_all failed\n");
 }*/

/* query a single process */
/*static void do_query_proc(struct localproc_struct *lproc, char *msg,
struct timeval *now) {
struct sockaddr_in addr ;
unsigned int pid ;
unsigned int gid ;
unsigned int group_available;
int retval ;

msg_parse_qryproc(msg, &pid, &gid, &group_available, &addr);

retval = proc_trust_list_member(lproc, pid, gid, group_available, &addr) ;

if(retval == 0)
  retval = CRASHED_NOTIF ;
if(retval == 1)
  retval = TRUST_NOTIF ;

retval = send_result(lproc, retval, 0, now) ;

if (retval < 0) {
#ifdef OUTPUT
fprintf(stderr, "fdd: do_query_proc failed\n") ;
#endif
#ifdef LOG
fprintf(flog, "fdd: do_query_proc failed\n") ;
#endif
}
 }*/

/* change the type of interruption desired by the local process */
/*static void do_interrupt_on_proc(struct localproc_struct *lproc, char *msg,
struct timeval *now) {
int error ;
u_int pid ;
struct sockaddr_in addr ;
u_int int_type ;
struct procqos_struct *pqos = NULL ;

msg_parse_int_proc(msg, &pid, &addr, &int_type);

pqos = locate_pqos(lproc, pid, &addr) ;

if( pqos )
  pqos->int_type = int_type ;

error = send_result(lproc, 0, 0, now);
if (error < 0) {
#ifdef OUTPUT
fprintf(stderr, "fdd: send_result_failed\n");
#endif
#ifdef LOG
fprintf(flog, "fdd: send_result_failed\n");
#endif
}
}

static void do_interrupt_on_group(struct localproc_struct *lproc, char *msg,
struct timeval *now) {
int error ;
u_int gid ;
u_int int_type ;

struct groupqos_struct *gqos = NULL ;

msg_parse_int_group(msg, &gid, &int_type);

gqos = locate_gqos(lproc, gid) ;

if( gqos )
  gqos->int_type = int_type ;

error = send_result(lproc, 0, 0, now);
if (error < 0) {
#ifdef OUTPUT
fprintf(stderr, "fdd: send_result_failed\n");
#endif
#ifdef LOG
fprintf(flog, "fdd: send_result_failed\n");
#endif
}
 }*/

/* build  the list of jointly groups between the remote host and the local */
extern int build_jointly_groups_list(struct host_struct  *host,
  struct list_head *all_remote_groups) {
  
  struct list_head *tmp ;
  struct uint_struct *group ;
  struct list_head jointly_groups ;
  
  int retval ;
  int entry_exists ;
  struct uint_struct *entry_ptr ;
  
  INIT_LIST_HEAD(&jointly_groups) ;
  
  retval = -ENOMEM ;
  list_for_each(tmp, all_remote_groups) {
    group = list_entry(tmp, struct uint_struct, uint_list) ;
    if(is_in_ordered_list(group->val, &local_groups_list_head)) {
      entry_ptr = NULL ;
      list_insert_ordered(group->val, val, &jointly_groups,
      struct uint_struct, uint_list, <) ;
      if(entry_ptr == NULL)
        goto out ;
    }
  }
  retval = 0 ;
  out:
  if( !(retval < 0) )
    list_swap(&jointly_groups, &host->jointly_groups_head) ;
  
  list_free(&jointly_groups, struct uint_struct, uint_list) ;
  
  return retval ;
}

/* a local process joins a group */
static int proc_join_group(struct localproc_struct *lproc, unsigned int gid,
  struct timeval *now) {
  
  int retval ;
  struct groupqos_struct *entry_ptr_gqos ;
  struct uint_struct *entry_ptr_uint ;
  int group_already_existed_localy;
  
  int entry_exists ;
  {
    struct groupqos_struct *entry_ptr ;
    retval = -ENOMEM ;
    entry_ptr = NULL ;
    list_insert_ordered(gid, gid, &lproc->gqlist_head, struct groupqos_struct,
    gqlist, <) ;
    
    if( entry_ptr == NULL )
      goto out ;
    
    entry_ptr->qos = NULL ;
    
    entry_ptr_gqos = entry_ptr ;
  }
  
  
  {
    struct uint_struct *entry_ptr ;
    
    entry_ptr = NULL ;
    entry_exists = 0 ;
    retval = -ENOMEM ;
    list_insert_ordered(gid, val, &local_groups_list_head, struct uint_struct,
    uint_list, <) ;
    if( entry_ptr == NULL )
      goto free_gqos ;
    
    group_already_existed_localy = entry_exists;
    entry_ptr_uint = entry_ptr ;
  }
  
  local_groups_list_seq++ ;
  local_groups_multicast_list_seq++ ;
  
  {
    struct list_head *tmp ;
    struct host_struct *host ;
    list_for_each(tmp, &remote_host_list_head) {
      host = list_entry(tmp, struct host_struct, remote_host_list) ;
      if(is_in_ordered_list(gid, &host->remote_all_groups_head)) {
        retval = build_jointly_groups_list(host, &host->remote_all_groups_head) ;
        if(retval < 0)
          goto free_uint ;
        
        if(host->stats.local_initial_finished == FINISHED_YES &&
          host->stats.remote_initial_finished == FINISHED_YES) {
          recalc_needed_sendint(host, now) ;
          local_sched_report_sooner(host->local_needed_sendint, now, host) ;
        }
      }
    }
  }
  
  if (!group_already_existed_localy) {
    sched_hello_multicast_now();
  }
  retval = 0 ;
  goto out ;
  
  free_uint:
  list_del(&entry_ptr_uint->uint_list) ;
  free(entry_ptr_uint) ;
  free_gqos:
  list_del(&entry_ptr_gqos->gqlist) ;
  free(entry_ptr_gqos) ;
  
  out:
  return retval ;
}

/* recompute the needed sendint value for all the remote hosts which have gid as jointly group */
static void recalc_needed_sendint_jointly_hosts(unsigned int gid, struct timeval *now) {
  
  struct host_struct *host ;
  struct list_head *tmp ;
  
  list_for_each(tmp, &remote_host_list_head) {
    host = list_entry(tmp, struct host_struct, remote_host_list) ;
    if(is_in_ordered_list(gid, &host->jointly_groups_head)) {
      recalc_needed_sendint(host, now) ;
      local_sched_report_sooner(host->local_needed_sendint, now, host) ;
    }
  }
}

/* stop monitoring a group */
extern int do_stop_monitor_group(unsigned int pid, unsigned int gid, struct timeval *now) {
  
  struct groupqos_struct *gqos ;
  int retval ;
  struct localproc_struct *lproc = NULL;
  int found;
  
  lproc = find_lproc(pid, &found);
  if (!found)
    return -1;
  
  retval = -EINVAL ;
  gqos = locate_gqos(lproc, gid) ;
  if(NULL == gqos || NULL == gqos->qos)
    goto out ;
  
  free(gqos->qos) ;
  gqos->qos = NULL ;
  
  recalc_needed_sendint_jointly_hosts(gid, now) ;
  
  retval = 0 ;
  out:
  if(retval < 0) {
#ifdef OUTPUT
    fprintf(stdout, "Error in stop monitor group\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Error in stop monitor group\n") ;
#endif
  }
  return retval;
}



/* Added for Omega */
/*stop sending alive messages */
extern int do_stop_sending_alives(unsigned int pid, unsigned int gid, struct timeval *now) {
  
  struct host_struct *host;
  struct uint_struct *group;
  struct list_head *tmp_head1, *tmp_head2;
  
#ifdef OUTPUT
  fprintf(stdout, "Stop sending alives for group: %u\n", gid);
#endif
#ifdef LOG
  fprintf(flog, "Stop sending alives for group: %u\n", gid);
#endif
  
  visibility_list_group_free(pid, gid);
  
  local_groups_list_seq++;
  
  
  /* Added for Omega */
  /* When a process stops sending alives send a report message
   to all the hosts that monitored this process by mean of group monitoring. */
  list_for_each(tmp_head1, &remote_host_list_head) {
    host = list_entry(tmp_head1, struct host_struct, remote_host_list);
    
    list_for_each(tmp_head2, &host->jointly_groups_head) {
      group = list_entry(tmp_head2, struct uint_struct, uint_list);
      if (group->val == gid) {
        if ((host->stats.local_initial_finished == FINISHED_YES) &&
          (host->stats.remote_initial_finished == FINISHED_YES)) {
          sched_report_now(host, now);
          break;
        }
      }
    }
  }
  return 0;
}

/* Added for Omega */
/* Restart sending alive messages */
extern int do_restart_sending_alives(unsigned int pid, unsigned int gid, struct timeval *now) {
  
  int retval ;
  int entry_exists = 0;
  struct host_struct *host;
  struct list_head *tmp_head1, *tmp_head2;
  struct uint_struct *group;
  struct list_head *tmp_visibility;
  struct pid_gid_visibility_struct *pid_gid;
  struct glist_struct *entry_ptr = NULL;
  
  
#ifdef OUTPUT
  fprintf(stdout, "Restart sending alives for group: %u\n", gid);
#endif
#ifdef LOG
  fprintf(flog, "Restart sending alives for group: %u\n", gid);
#endif
  
  
  retval = -ENOMEM;
  
  list_for_each(tmp_visibility, &visibility_list_head) {
    pid_gid = list_entry(tmp_visibility, struct pid_gid_visibility_struct, pid_list_head);
    if (pid == pid_gid->pid) {
      list_insert_ordered(gid, gid, &pid_gid->gid_list_head, struct glist_struct,
      gcell, <) ;
      break;
    }
  }
  
  if (entry_ptr == NULL)
    goto out;
  
  local_groups_list_seq++ ;
  
  /* Added for Omega */
  /* Update the group ts. */
  if (add_update_group_ts(gid, now) < 0) {
    fprintf(stderr, "fdd: Error in do_restart_sending_alives while adding or updating group ts\n");
    goto out;
  }
  
  /* Added for Omega */
  /* When a process restarts sending alives we send a report message
   to all the hosts that monitored this process by mean of group monitoring. */
  list_for_each(tmp_head1, &remote_host_list_head) {
    host = list_entry(tmp_head1, struct host_struct, remote_host_list);
    
    list_for_each(tmp_head2, &host->jointly_groups_head) {
      group = list_entry(tmp_head2, struct uint_struct, uint_list);
      if (group->val == gid) {
        if ((host->stats.local_initial_finished == FINISHED_YES) &&
          (host->stats.remote_initial_finished == FINISHED_YES)) {
          sched_report_now(host, now);
          break;
        }
      }
    }
  }
  
  
  retval = 0 ;
  
  out:
  if(retval < 0) {
#ifdef OUTPUT
    fprintf(stdout, "Error in restart sending alives\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Error in restart sending alives\n") ;
#endif
  }
  return retval;
}



/* set the group qos in the monitoring of a local process and a group */
static int set_gqos(struct localproc_struct *lproc, unsigned int gid,
  u_int int_type,
  unsigned int TdU, unsigned int TmU, unsigned int TmrL,
  struct timeval *now) {
  
  int retval ;
  
  struct groupqos_struct *gqos ;
  
  retval = -EINVAL ;
  gqos = locate_gqos(lproc, gid) ;
  if(NULL == gqos)
    goto out ;
  if(NULL == gqos->qos ) {
    retval = -ENOMEM ;
    gqos->qos = malloc(sizeof(*(gqos->qos))) ;
    if(NULL == gqos->qos)
      goto out ;
    local_trust_group(lproc, gid, now) ;
  }
  
  gqos->int_type = int_type ;
  
  gqos->qos->TdU = TdU ;
  gqos->qos->TmU = TmU ;
  gqos->qos->TmrL = TmrL ;
  
  retval = 0 ;
  out:
  return retval ;
}

/* start monitoring a group */
extern int do_monitor_group(unsigned int pid, unsigned int gid, unsigned int int_type, unsigned int TdU,
  unsigned int TmU, unsigned int TmrL, struct timeval *now) {
  
  int retval ;
  
  struct list_head *tmp_host;
  struct host_struct *host;
  struct localproc_struct *lproc = NULL;
  int found;
  
  lproc = find_lproc(pid, &found);
  if (!found)
    return -1;
  
  
  retval = set_gqos(lproc, gid, int_type, TdU, TmU, TmrL, now) ;
  if(retval < 0)
    goto out ;
  
  recalc_needed_sendint_jointly_hosts(gid, now) ;
  
  /* Added for Omega */
  /* Step 3 of Handshake is accelerated:
  When we receive the monitor group command we send a report messages to all hosts that have a group
   equal to gid. */
  list_for_each(tmp_host, &remote_host_list_head) {
    host = list_entry(tmp_host, struct host_struct, remote_host_list);
    if (is_in_ordered_list(gid, &host->jointly_groups_head)) {
      if ((host->stats.local_initial_finished == FINISHED_YES) &&
        (host->stats.remote_initial_finished == FINISHED_YES)) {
#ifdef OUTPUT
        fprintf(stdout, "Handshake step 3 case b) (monitor group after hello back): send a report msg now!\n") ;
#endif
#ifdef LOG
        fprintf(flog, "Handshake step 3 case b) (monitor group after hello back): send a report msg now!\n") ;
#endif
        
        sched_report_now(host, now);
      }
    }
  }
  
  
  retval = 0 ;
  out:
  if(retval < 0) {
#ifdef OUTPUT
    fprintf(stdout, "Error in monitor_group\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Error in monitor_group\n") ;
#endif
  }
  return retval;
}


/* joining a group */
extern int do_join_group(unsigned int pid, unsigned int gid, struct timeval *now) {
  
  int retval = 0, entry_exists ;
  struct list_head *tmp_visibility;
  struct pid_gid_visibility_struct *pid_gid = NULL;
  struct localproc_struct *lproc = NULL;
  int found;
  
  lproc = find_lproc(pid, &found);
  if (!found)
    return -1;
  
  
  /* Added for Omega */
  /* Adds the variables for group gid in the localvars_list. We don't have to do that
   for the FDD group. */
  if (gid != 0) {
    if (!exists_group(gid)) {
      retval = add_group_ts(gid, now);
      if (retval < 0)
        goto out;
    }
    
    if (!localvars_exist(gid)) {
      retval = create_localvars(gid);
      if (retval < 0)
        goto out;
    }
  }
  
  
  retval = proc_join_group(lproc, gid, now) ;
  
  if(retval < 0)
    goto out ;
  
  
  if (gid != 0) {
    /* Added for Omega */
    /* Adds the group gid to the process lproc->pid in the visibility_list */
    /* We don't have to add that for the FDD GROUP */
    struct glist_struct *entry_ptr ;
    
    entry_ptr = NULL;
    entry_exists = 0;
    retval = -ENOMEM;
    
    list_for_each(tmp_visibility, &visibility_list_head) {
      pid_gid = list_entry(tmp_visibility, struct pid_gid_visibility_struct, pid_list_head);
      if (lproc->pid == pid_gid->pid) {
        list_insert_ordered(gid, gid, &pid_gid->gid_list_head, struct glist_struct,
        gcell, <) ;
        break;
      }
    }
    
    if (entry_ptr == NULL)
      goto out;
  }
  
  retval = notify_local_group(lproc->pid, gid, TRUST_NOTIF, now) ;
  
  out:
  
  if(retval < 0) {
#ifdef OUTPUT
    fprintf(stdout, "Error in join group\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Error in join group\n") ;
#endif
  }
  
  return retval;
}

/* Added for Omega */
/* Join a group but be invisible from the begining in that group. It means that we don't
 add the group in the pid_gid visibility list. */
extern int do_join_group_invisible(unsigned int pid, unsigned int gid, int candidate,
  struct timeval *now) {
  
  int retval = 0 ;
  struct localproc_struct *lproc = NULL;
  int found;
  
  lproc = find_lproc(pid, &found);
  if (!found)
    return -1;
  
  
  /* Added for Omega */
  /* Adds the variables for group gid in the localvars_list. We don't have to do that
   for the FDD group. */
  if (!exists_group(gid)) {
    retval = add_group_ts(gid, now);
    if (retval < 0)
      goto out;
  }
  
  if ((candidate == CANDIDATE) && !localvars_exist(gid)) {
    retval = create_localvars(gid);
    if (retval < 0)
      goto out;
  }
  
  retval = proc_join_group(lproc, gid, now) ;
  
  if(retval < 0)
    goto out ;
  
  retval = notify_local_group(lproc->pid, gid, TRUST_NOTIF, now) ;
  
  out:
  
  if(retval < 0) {
#ifdef OUTPUT
    fprintf(stdout, "Error in join group invisible\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Error in join group invisible\n") ;
#endif
  }
  return retval;
}



/* leaving a group */
extern int do_leave_group(unsigned int pid, unsigned int gid, struct timeval *now) {
  
  struct groupqos_struct *gqos = NULL ;
  struct localproc_struct *lproc = NULL;
  int found;
  
  int retval ;
  
  lproc = find_lproc(pid, &found);
  if (!found)
    return -1;
  
  gqos = locate_gqos(lproc, gid) ;
  
  retval = -EINVAL ;
  if(gqos == NULL)
    goto out ;
  
  list_del(&gqos->gqlist) ;
  proc_quit_group(lproc, gqos->gid, now) ;
  free_gqos(gqos) ;
  
  retval = 0 ;
  out:
  if(retval < 0) {
#ifdef OUTPUT
    fprintf(stdout, "Error in leave group\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Error in leave group\n") ;
#endif
  }
  return retval;
}


/* Build the list of remote servers processes that I need service from */
static int local_build_remote_servers_proc_list(struct host_struct *host,
  struct list_head *remote_servers_proc_list) {
  struct list_head *tmp_pqos        = NULL ;
  struct procqos_struct *pqos       = NULL ;
  
  struct list_head *tmp_client      = NULL ;
  struct uint_struct *local_client_pid  = NULL ;
  struct localproc_struct *lproc    = NULL ;
  
  int retval ;
  
  retval = 0 ;
  /* check for each local client for that host */
  list_for_each(tmp_client, &host->local_clients_proc_head) {
    local_client_pid = list_entry(tmp_client, struct uint_struct, uint_list) ;
    lproc = get_local_proc(local_client_pid->val) ;
    
    /* search the remote server processes in that host */
    list_for_each(tmp_pqos, &lproc->pqlist_head) {
      pqos = list_entry(tmp_pqos, struct procqos_struct, pqlist) ;
      if( sockaddr_eq(&pqos->addr, &host->addr) ) {
        struct uint_struct *entry_ptr = NULL ;
        int entry_exists ;
        retval = -ENOMEM ;
        list_insert_ordered(pqos->pid, val, remote_servers_proc_list,
        struct uint_struct, uint_list, <) ;
        if(entry_ptr == NULL)
          goto out ;
      }
    }
  }
  
  retval = 0 ;
  out:
  if(retval < 0) {
#ifdef OUTPUT
    fprintf(stdout, "Error in local_build_remote_servers_proc_list\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Error in local_build_remote_servers_proc_list\n") ;
#endif
  }
  return retval ;
}
/* check if the val is in the ordered list */
extern int is_in_ordered_list(u_int val, struct list_head *o_list) {
  struct uint_struct *el ;
  struct list_head *tmp = NULL ;
  list_for_each(tmp, o_list) {
    el = list_entry(tmp, struct uint_struct, uint_list) ;
    if(el->val == val)
      return 1 ;
    if(el->val > val)
      break ;
  }
  return 0 ;
}
/* create the fdd's local process and group */
static int create_fdd_proc_group(struct timeval *now) {
  
  int retval ;
  struct localproc_struct *entry_ptr ;
  int entry_exists ;
  
  retval = -ENOMEM ;
  entry_ptr = NULL ;
  list_insert_ordered(FDD_PID, pid, &local_procs_list_head,
  struct localproc_struct, local_procs_list, <) ;
  
  if(entry_ptr == NULL)
    goto out ;
  
  INIT_LIST_HEAD(&entry_ptr->pqlist_head) ;
  INIT_LIST_HEAD(&entry_ptr->gqlist_head) ;
  INIT_LIST_HEAD(&entry_ptr->tmp_tlist_head) ;
  INIT_LIST_HEAD(&entry_ptr->trust_list_head) ;
  
  /*  entry_ptr->int_type = INT_NONE ; */
  
  if (add_group_ts(FDD_GID, now) < 0)
    fprintf(stderr, "fdd: Error impossible to add group ts of FDD_GROUP\n");
  
  retval = proc_join_group(entry_ptr, FDD_GID, now) ;
  if(retval < 0)
    goto out ;
  
  retval = set_gqos(entry_ptr, FDD_GID, INTERRUPT_NONE,
  FDD_GROUP_TDU, FDD_GROUP_TMU, FDD_GROUP_TMRL, now) ;
  if(retval < 0)
    goto out ;
  
  retval = 0 ;
  out:
  if(retval < 0) {
#ifdef OUTPUT
    fprintf(stdout, "Could not create the fdd_proc_group\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Could not create the fdd_proc_group\n") ;
#endif
  }
  return retval ;
}

/*
 * initialize local_epoch timestamp, local_seq and local_list_seq.
 * initialize the list of local processes.
 */
extern void local_init(void) {
  gettimeofday(&local_epoch, NULL) ;
  srand((u_int)local_epoch.tv_usec) ;
  
  /* init the local proces list */
  INIT_LIST_HEAD(&local_procs_list_head) ;
  
  /* init the local groups list */
  INIT_LIST_HEAD(&local_groups_list_head) ;
  
  /* Added for Omega */
  /* init the visibility list */
  INIT_LIST_HEAD(&visibility_list_head);
  
  /* Added for Omega */
  /* init the group_ts list */
  INIT_LIST_HEAD(&group_join_ts_head);
  
  timerclear(&last_report_ts) ;
  timerclear(&next_report_ts) ;
  
  local_groups_list_seq = 0 ;
  local_groups_multicast_list_seq = 0 ;
  
  create_fdd_proc_group(&local_epoch) ;
  
  local_init_multicast() ;
}

/* locate a proc qos definition */
extern struct procqos_struct *locate_pqos(struct localproc_struct *lproc,
  unsigned int pid,
  struct sockaddr_in *addr) {
  struct list_head *tmp       = NULL ;
  struct procqos_struct *pqos = NULL ;
  
  list_for_each(tmp, &lproc->pqlist_head) {
    pqos = list_entry(tmp, struct procqos_struct, pqlist) ;
    if( pid == pqos->pid && sockaddr_eq(&pqos->addr, addr) ) {
      goto out;
    }
    if( pid < pqos->pid )
      break ;
  }
  pqos = NULL ;
  out:
  return pqos ;
}

/* locate a group qos definition */
extern struct groupqos_struct *locate_gqos(struct localproc_struct *lproc, unsigned int gid) {
  
  struct list_head *tmp_gqos = NULL ;
  struct groupqos_struct *gqos = NULL ;
  
  list_for_each(tmp_gqos, &lproc->gqlist_head) {
    gqos = list_entry(tmp_gqos, struct groupqos_struct, gqlist) ;
    if(gqos->gid < gid)
      continue ;
    if(gqos->gid == gid)
      goto out ;
    break ;
  }
  
  gqos = NULL ;
  out:
  return gqos ;
}

/* remove all procs from that host from the trust list,
if host == NULL, remove regardless of host,
 if remove_list != NULL, then move all to remove_list instead of removing */
extern void local_untrust_host(struct localproc_struct *lproc,
  struct host_struct *host,
  struct list_head *remove_list) {
  struct trust_struct *tproc = NULL ;
  struct list_head *tmp = NULL ;
  
  if(remove_list != NULL)
    INIT_LIST_HEAD(remove_list) ;
  
  sched_unsched_host(lproc, host) ;
  list_for_each(tmp, &lproc->trust_list_head) {
    tproc = list_entry(tmp, struct trust_struct, trust_list);
    if(host != NULL && !sockaddr_eq(&host->addr, &tproc->host->addr))
      continue ;
    
    tmp = tmp->prev ;
    list_del(&tproc->trust_list) ;
    if(remove_list != NULL)
      list_add(&tproc->trust_list, remove_list) ;
    else
      free_trust(tproc) ;
  }
  return ;
}
/* register a local process */
extern int fdd_local_reg(unsigned int pid, struct timeval *reg_ts) {
  struct localproc_struct *entry_ptr ;
  int entry_exists ;
  
  struct host_struct *rhost;
  struct list_head *tmp_host;
  int retval ;
  
  /*  printf("Received register command from pid =%u\n", pid) ;*/
  
  retval = -EPERM ;
  
  if(isalive(pid)) {
#if OUTPUT
    fprintf(stdout, "The pid %u already exists. Fatal error, exiting application.\n", pid) ;
#endif
#ifdef LOG
    fprintf(flog, "The pid %u already exists. Fatal error, exiting application.\n", pid) ;
#endif
    exit(-1) ;
  }
  
  entry_ptr = NULL ;
  retval = -ENOMEM ;
  list_insert_ordered(pid, pid, &local_procs_list_head, struct localproc_struct,
  local_procs_list, <) ;
  if(entry_ptr == NULL)
    goto error ;
  
  /* Added for Omega */
  /* Add the process pid in the visibility list */
  {
    struct pid_gid_visibility_struct *entry_ptr = NULL;
    retval = -ENOMEM;
    list_insert_ordered(pid, pid, &visibility_list_head, struct pid_gid_visibility_struct,
    pid_list_head, <) ;
    entry_exists = 0;
    if (entry_ptr == NULL)
      goto error;
    
    INIT_LIST_HEAD(&(entry_ptr->gid_list_head));
  }
  
  
  INIT_LIST_HEAD(&entry_ptr->pqlist_head) ;
  INIT_LIST_HEAD(&entry_ptr->gqlist_head) ;
  INIT_LIST_HEAD(&entry_ptr->tmp_tlist_head) ;
  
  
  INIT_LIST_HEAD(&entry_ptr->trust_list_head) ;
  notify_local(entry_ptr->pid, &fdd_local_addr, TRUST_NOTIF, reg_ts) ;
  
  reg_lproc_in_related_hosts(entry_ptr->pid, reg_ts) ;
  
#ifdef OUTPUT
  fprintf(stdout, "LocalProc %u Registered, TS=%ld.%ld\n", pid,
  reg_ts->tv_sec, reg_ts->tv_usec);
#endif
#ifdef LOG
  fprintf(flog, "LocalProc %u Registered, TS=%ld.%ld\n", pid,
  reg_ts->tv_sec, reg_ts->tv_usec);
#endif
  
  /* Added for Omega*/
  /* When a local process register we send a report to every host which monitors
  proc pid.
   This way remote hosts notice faster that a crashed process has recovered. */
  list_for_each(tmp_host, &remote_host_list_head) {
    rhost = list_entry(tmp_host, struct host_struct, remote_host_list);
    if (is_in_ordered_list(pid, &rhost->local_servers_proc_head)) {
      if ((rhost->stats.local_initial_finished == FINISHED_YES) &&
      (rhost->stats.remote_initial_finished == FINISHED_YES))
      sched_report_now(rhost, reg_ts);
    }
  }
  return 0;
  
  error:
#ifdef OUTPUT
  fprintf(stderr, "fdd: fdd_local_reg failed %d\n", retval) ;
#endif
#ifdef LOG
  fprintf(flog, "fdd: fdd_local_reg failed %d\n", retval) ;
#endif
  return retval;
}


/* Added for Omega */
/* Checks if the group gid is in the visibility for process pid */
static int is_in_ordered_visibility_list(u_int pid, u_int gid) {
  
  struct list_head *tmp_visibility;
  struct pid_gid_visibility_struct *pid_gid;
  struct list_head *tmp_gid;
  struct glist_struct *gid_struct;
  
  list_for_each(tmp_visibility, &visibility_list_head) {
    pid_gid = list_entry(tmp_visibility, struct pid_gid_visibility_struct, pid_list_head);
    if (pid_gid->pid < pid)
      continue;
    if (pid_gid->pid == pid) {
      list_for_each(tmp_gid, &pid_gid->gid_list_head) {
        gid_struct = list_entry(tmp_gid, struct glist_struct, gcell);
        if (gid_struct->gid < gid)
          continue;
        if (gid_struct->gid == gid) {
          return 1;
        }
        break;
      }
      return 0;
    }
    break;
  }
  return -1;
}


/* sends a report message to the given host timestamped with the given sending_ts
 * the report message has two main components:
 *  - the list of local servers. All the process servers requested from this host are in
 *    the field host->local_servers_proc_list.
 *  - the list of remote servers processes in that host, that are monitored. To find out
 *    the list of remote servers, we chech the list of local clients that request any
 *    service from that host: host->local_clients_proc_list, and we pick up the remote
 * processes requested for service.
 */

/* Also send in the message the host->remote_needed_sendints_list */
extern void local_send_report_host(struct host_struct *host,
  struct timeval *sending_ts) {
  char msg[SAFE_MSG_LEN] ;
  char *ptr       = NULL ;
  char *ptr_head  = NULL ;
  char *ptr_ghead = NULL ;
  char *ptr_list  = NULL ;
  
  int local_servers_proc_count   = 0 ;
  int local_servers_list_len     = 0 ;
  int remote_servers_proc_count  = 0 ;
  int remote_servers_list_len    = 0 ;
  
  int local_servers_groups_count = 0 ;
  unsigned int localvars_count = 0;
  
  struct uint_struct *proc_pid = NULL ;
  /* the list of remote servers that we will build */
  struct list_head remote_servers_proc_head ;
  struct list_head *tmp          = NULL ;
  
  struct list_head *tmp_jointly_groups = NULL ;
  struct list_head *tmp_lprocs = NULL ;
  
  struct uint_struct *jointly_group = NULL ;
  struct localproc_struct *lproc = NULL ;
  
  unsigned int procs_count ;
  struct timeval accusationTime, bestAmongActives_accTime;
  struct bestAmongActives_struct bestAmongActives;
  int exists_inv_proc_in_group;
  
  int retval ;
  
  retval =  0 ;
  
  
  INIT_LIST_HEAD(&remote_servers_proc_head) ;
  
#ifdef OUTPUT
  fprintf(stdout, "SENDING REPORT seq=%u, local_needed_sendint=%u, "
    "to %u.%u.%u.%u, TS=%ld.%ld\n",
    host->local_seq+1, host->local_needed_sendint,
    NIPQUAD(&host->addr),
  sending_ts->tv_sec, sending_ts->tv_usec) ;
#endif
#ifdef LOG_EXTRA
  fprintf(flog, "SENDING REPORT seq=%u, local_sendint=%u, "
    "to %u.%u.%u.%u. TS=%ld.%ld\n",
    host->local_seq+1, host->local_needed_sendint,
    NIPQUAD(&host->addr),
  sending_ts->tv_sec, sending_ts->tv_usec) ;
#endif
  ptr = msg ;
  ptr_head = ptr ;
  
  /* skip the head of the message */
  retval = -EMSGSIZE ;
  if((ptr = msg_skip_rep_head(ptr)) > msg + MAX_MSG_LEN) {
#ifdef OUTPUT
    fprintf(stderr, "Buffer overflow\n") ;
#endif
#ifdef LOG
    fprintf(flog, "Buffer overflow\n") ;
#endif
    goto out;
  }
  
  ptr_list = ptr;
  
  retval = -EMSGSIZE ;
  local_servers_proc_count = 0 ;
  list_for_each(tmp, &host->local_servers_proc_head) {
    proc_pid = list_entry(tmp, struct uint_struct, uint_list) ;
    if(!isalive(proc_pid->val)) {
      continue ;
    }
    ptr = msg_build_rep_pid(ptr, proc_pid->val) ;
    if(ptr > msg + MAX_MSG_LEN) {
#ifdef OUTPUT
      fprintf(stderr, "Buffer overflow\n") ;
#endif
#ifdef LOG
      fprintf(flog, "Buffer overflow\n") ;
#endif
      goto out ;
    }
    local_servers_proc_count++ ;
  }
  
  
  
  /* Build the list of local groups for the report message */
  local_servers_groups_count = 0 ;
  retval = -EMSGSIZE ;
  list_for_each(tmp_jointly_groups, &host->jointly_groups_head) {
    jointly_group = list_entry(tmp_jointly_groups, struct uint_struct, uint_list) ;
    procs_count = 0 ;
    exists_inv_proc_in_group = 0;
    ptr_ghead = ptr ;
    
    list_for_each(tmp_lprocs, &local_procs_list_head) {
      lproc = list_entry(tmp_lprocs, struct localproc_struct, local_procs_list);
      
      if(is_in_ordered_gqlist(jointly_group->val, &lproc->gqlist_head)) {
        /* Added for Omega */
        /* Checks if we are invisible in group gid */
        if (jointly_group->val != 0) {
          int found = is_in_ordered_visibility_list(lproc->pid, jointly_group->val);
          if (found == 0) {
            exists_inv_proc_in_group = 1;
            continue ;
          }
          else if (found == -1)
            continue;
        }
        
        if(!procs_count) { /* first proc added --> skip the group head */
          ptr = msg_skip_rep_ghead(ptr) ;
          if(ptr > msg + MAX_MSG_LEN) {
            fprintf(stderr, "local_send_report: fatal buffer overflow error\n");
            goto out ;
          }
        }
        procs_count++ ;
        ptr = msg_build_rep_pid(ptr, lproc->pid) ;
        if(ptr > msg + MAX_MSG_LEN) {
          fprintf(stderr, "local_send_report: fatal buffer overflow error\n");
          goto out ;
        }
      }
    }
    
    if(procs_count) {
      char *tmp_ptr;
      struct timeval group_ts;
      unsigned int eta_rcvd;
      
      if (get_group_ts(jointly_group->val, &group_ts) < 0) {
        fprintf(stderr, "fdd: Error in local_send_report, impossible to get group ts of: %u\n",
        jointly_group->val);
        goto out;
      }
      
      eta_rcvd = timercmp(&host->local_largest_group_ts_rcvd, &group_ts, ==) ||
      timercmp(&host->local_largest_group_ts_rcvd, &group_ts, >);
      
      tmp_ptr = msg_build_rep_ghead(ptr_ghead, jointly_group->val,
      &group_ts, eta_rcvd, procs_count) ;
      local_servers_groups_count++ ;
    }
    /* Modified for Omega */
    /* We include a group g even if there is no visible process in g. */
    else if(exists_inv_proc_in_group) {
      char *tmp_ptr;
      struct timeval group_ts;
      unsigned int eta_rcvd = 0;
      
      /* If there is no group ts for jointly_group->val, we assign
       zero to group_ts. */
      if (get_group_ts(jointly_group->val, &group_ts) < 0)
        timerclear(&group_ts);
      
      tmp_ptr = msg_build_rep_ghead(ptr_ghead, jointly_group->val,
      &group_ts, eta_rcvd, procs_count) ;
      local_servers_groups_count++ ;
      
      /* We built the group head without any process in it.
       We must therefore update the ptr var. */
      ptr = tmp_ptr;
    }
  }
  
  local_servers_list_len = ptr-ptr_list ;
  
  /* Build the list of remote servers processes that I need service from */
  retval = local_build_remote_servers_proc_list(host, &remote_servers_proc_head) ;
  if(retval < 0) {
    goto out ;
  }
  
  /* add the list of remote servers to the report message */
  ptr_list = ptr ;
  ptr_ghead = ptr ;
  remote_servers_proc_count = 0 ;
  
  list_for_each (tmp, &remote_servers_proc_head) {
    proc_pid = list_entry(tmp, struct uint_struct, uint_list) ;
    ptr = msg_build_rep_pid(ptr, proc_pid->val) ;
    tmp = tmp->prev ;
    list_del(&proc_pid->uint_list) ;
    
    free(proc_pid) ;
    
    retval = -EMSGSIZE ;
    if(ptr > msg + MAX_MSG_LEN) {
#ifdef OUTPUT
      fprintf(stderr, "Buffer overflow\n") ;
#endif
#ifdef LOG
      fprintf(flog, "Buffer overflow\n") ;
#endif
      goto out ;
    }
    remote_servers_proc_count++;
  }
  remote_servers_list_len = ptr-ptr_list ;
  
  
  /* Added for Omega */
  /* Add the variables accusationTime and startTime of processes belonging to the jointly groups */
  list_for_each(tmp_jointly_groups, &host->jointly_groups_head) {
    jointly_group = list_entry(tmp_jointly_groups, struct uint_struct, uint_list) ;
    list_for_each(tmp_lprocs, &local_procs_list_head) {
      lproc = list_entry(tmp_lprocs, struct localproc_struct, local_procs_list);
      if(is_in_ordered_gqlist(jointly_group->val, &lproc->gqlist_head) &&
        is_in_ordered_visibility_list(lproc->pid, jointly_group->val) && (jointly_group->val != 0)) {
        if (getlocalvars(jointly_group->val, &accusationTime, &bestAmongActives) == 0) {
          if (sockaddr_eq(&fdd_local_addr, &(bestAmongActives.addr)))
          ptr = msg_build_rep_localvars(ptr, jointly_group->val, &accusationTime, &bestAmongActives,
          &accusationTime);
          else {
            if (get_accusationTime_of_remoteprocess(&(bestAmongActives.addr),
                jointly_group->val,
              &bestAmongActives_accTime) < 0) {
              fprintf(stderr, "fdd: Error in local_send_report couldn't find remotevars\n");
              fprintf(stderr, "of machine: %u.%u.%u.%u\n", NIPQUAD(&(bestAmongActives.addr)));
              break;
            }
            else {
              ptr = msg_build_rep_localvars(ptr, jointly_group->val, &accusationTime, &bestAmongActives,
              &bestAmongActives_accTime);
            }
          }
          localvars_count++;
          break;   /* There is only one set of localvars per group */
        }
      }
    }
  }
  
  host->local_seq++ ;
  
  msg_build_rep_head(ptr_head, sending_ts, host->local_seq,
    &local_epoch, &host->remote_epoch,
    
    host->local_servers_list_seq, local_groups_list_seq,
    local_servers_list_len, local_servers_proc_count,
    local_servers_groups_count, host->local_needed_sendint,
    localvars_count,
    
    host->local_clients_list_seq, remote_servers_list_len,
    remote_servers_proc_count, host->remote_needed_sendint,
  &host->remote_largest_group_ts_rcvd) ;
  
  retval = comm_send(msg, ptr-msg, &host->addr) ;
  
  if (retval >= 0)
    memcpy(&host->last_report_ts, sending_ts, sizeof(host->last_report_ts));
  
  if(host->local_needed_sendint)
    unit2timer(host->local_needed_sendint, &host->next_report_ts) ;
  else
    fprintf(stderr, "Error: send interval to host: %u.%u.%u.%u is 0!\n", NIPQUAD(&host->addr));
  
  timeradd(&host->last_report_ts, &host->next_report_ts, &host->next_report_ts) ;
  sched_report(host) ;
  
  out:
  if (retval < 0) {
#ifdef OUTPUT
    fprintf(stderr, "fdd: local_send_report_host() failed\n");
#endif
#ifdef LOG
    fprintf(flog, "fdd: local_send_report_host() failed\n");
#endif
    list_free(&remote_servers_proc_head, struct uint_struct, uint_list) ;
  }
}

/* make sure next report is send no later than sendint after the last one.
 if we are late, but put timestamp now */
extern void local_sched_report_sooner(u_int sendint, struct timeval *now,
  struct host_struct *host) {
  struct timeval tv ;
  
  /*
  if(!sendint)
    goto out ;
   */
  
#ifdef OUTPUT
  print_ts(now);
#endif
  
  unit2timer(sendint, &tv) ;
  timeradd(&host->last_report_ts, &tv, &tv);
  if(timercmp(&tv, &host->next_report_ts, <)) {
    //    if(timercmp(&tv, now, <)) {
    memcpy(&host->next_report_ts, now, sizeof(*now));
    sched_report(host);
    //    }
  }
  //    out:
  return ;
}
/* send a suspect notif to all local processes for the given lproc */
extern void local_suspect(struct localproc_struct *lproc,
  struct trust_struct *tproc,
  struct timeval *suspect_ts) {
  
  list_del(&tproc->trust_list);
  if(INTERRUPT_ANY_CHANGE == tproc->int_type) {
    local_change_notify(lproc, SUSPECT_NOTIF, tproc, suspect_ts) ;
    
    tproc->host->remote_groups_list_seq--;
  }
  
  free_trust(tproc);
}

/* send a notification to all local processes */
extern void local_change_notify(struct localproc_struct *lproc, int not_type,
  struct trust_struct *tproc,
  struct timeval *notify_ts) {
  
  int retval;
  
#ifdef OUTTRUST
  char not_trust[] = "TRUST_NOTIF" ;
  char not_suspect[] = "SUSPECT_NOTIF" ;
  char not_crashed[] = "CRASHED_NOTIF" ;
#endif
  retval = 0;
  
#ifdef OUTTRUST
  printf("Notice %s to %u, for %u on ",
    not_type==TRUST_NOTIF?not_trust:not_type==SUSPECT_NOTIF?not_suspect:
  not_crashed, lproc->pid, tproc->pid) ;
  if(tproc->host)
    printf("%u.%u.%u.%u ", NIPQUAD(&tproc->host->addr)) ;
  else
    printf("local host ") ;
#endif
  
  if(tproc->int_type == INTERRUPT_ANY_CHANGE) {
    if (tproc->gid != NULL) {
      if ((not_type == SUSPECT_NOTIF) || (not_type == CRASHED_NOTIF)) {
        if (tproc->host != NULL)
          doUponSuspected(&tproc->host->addr, tproc->pid, tproc->gid->val, notify_ts);
        else
          doUponSuspected(&fdd_local_addr, tproc->pid, tproc->gid->val, notify_ts);
      }
    }
    
#ifdef OUTTRUST
    printf("Notice Sent") ;
    fprintf(stdout, "\n---Sent notification to ") ;
#endif
#ifdef LOG
    fprintf(flog, "\n---Sent notification to ") ;
#endif
  }
  
#ifdef OUTTRUST
  printf("\n") ;
#endif
  
  if(retval < 0) {
#ifdef OUTTRUST
    fprintf(stderr, "fdd: local_change_notify failed\n");
#endif
#ifdef LOG
    fprintf(flog, "fdd: local_change_notify failed\n");
#endif
  }
  
#ifdef OUTTRUST
  fprintf(stdout, "local proc: %u %s proc: %u on %u.%u.%u.%u ", lproc->pid,
    not_type==TRUST_NOTIF?"trusts":not_type==SUSPECT_NOTIF?
    "suspects":"crashes", tproc->pid,
  NIPQUAD(&(tproc->host?tproc->host->addr:fdd_local_addr))) ;
  if(tproc->gid)
    fprintf(stdout, "Group: %u\n", tproc->gid->val) ;
  else
    fprintf(stdout, "Pt. to Pt. monitoring\n") ;
#endif
#ifdef LOG
  fprintf(flog, "local proc: %u %s proc: %u on %u.%u.%u.%u ", lproc->pid,
    not_type==TRUST_NOTIF?"trusts":not_type==SUSPECT_NOTIF?
    "suspects":"crashes", tproc->pid,
  NIPQUAD(&(tproc->host?tproc->host->addr:fdd_local_addr))) ;
  if(tproc->gid)
    fprintf(flog, "Group: %u\n", tproc->gid->val) ;
  else
    fprintf(flog, "Pt. to Pt. monitoring\n") ;
#endif
  
  
  return ;
}

/* get a pointer to the local proces having the given pid */
extern struct localproc_struct* get_local_proc(unsigned int pid) {
  struct list_head *tmp          = NULL ;
  struct localproc_struct *lproc = NULL ;
  
  list_for_each(tmp, &local_procs_list_head) {
    lproc = list_entry(tmp, struct localproc_struct, local_procs_list) ;
    if( lproc->pid < pid )
      continue ;
    
    if( lproc->pid == pid )
      goto out ;
    
    break ;
  }
  lproc = NULL ;
  out:
  return lproc ;
}

/* add a new client  in the local client list of processes for the given host */
extern int add_local_client(struct host_struct *host,
  struct localproc_struct *lproc) {
  struct uint_struct *entry_ptr ;
  int entry_exists ;
  int retval ;
  
  retval = 0 ;
  
  entry_ptr = NULL ;
  list_insert_ordered(lproc->pid, val, &host->local_clients_proc_head,
  struct uint_struct, uint_list, <);
  
  retval = -ENOMEM ;
  if(entry_ptr == NULL)
    goto out ;
  
  retval = 0 ;
  out:
  return retval ;
}
/* build the list of local groups */
extern int build_groups_list(struct list_head *groups_list) {
  
  struct list_head *tmp_lproc ;
  struct list_head *tmp_gqos ;
  
  struct localproc_struct *lproc = NULL ;
  struct groupqos_struct *gqos = NULL ;
  struct uint_struct *entry_ptr ;
  int entry_exists ;
  int retval ;
  
  INIT_LIST_HEAD(groups_list) ;
  retval = -ENOMEM ;
  
  list_for_each(tmp_lproc, &local_procs_list_head) {
    lproc = list_entry(tmp_lproc, struct localproc_struct, local_procs_list) ;
    
    list_for_each(tmp_gqos, &lproc->gqlist_head) {
      gqos = list_entry(tmp_gqos, struct groupqos_struct, gqlist) ;
      
      entry_ptr = NULL ;
      list_insert_ordered(gqos->gid, val, groups_list, struct uint_struct,
      uint_list, <) ;
      
      if(entry_ptr == NULL)
        goto out ;
    }
  }
  
  retval = 0 ;
  out:
  if( retval < 0 ) {
    list_free(groups_list, struct uint_struct, uint_list) ;
#ifdef OUTPUT
    fprintf(stdout, "OUT OF MEMORY\n") ;
#endif
#ifdef LOG
    fprintf(flog, "OUT OF MEMORY\n") ;
#endif
  }
  return retval ;
}

/* management function: shows all local processes and additional
 information about them */
extern void show_localprocs() {
  
  struct list_head *tmp_lproc    = NULL ;
  struct localproc_struct *lproc = NULL ;
  
  struct list_head *tmp_pqos     = NULL ;
  struct procqos_struct *pqos    = NULL ;
  
  struct list_head *tmp_host     = NULL ;
  struct host_struct *host       = NULL ;
  
  struct list_head *tmp_gqos = NULL ;
  struct groupqos_struct *gqos = NULL ;
  
  struct list_head *tmp_trust ;
  struct trust_struct *trust ;
  
  {
    struct list_head *tmp_group = NULL ;
    struct uint_struct *group = NULL ;
    
    printf("Local groups:") ;
    list_for_each(tmp_group, &local_groups_list_head) {
      group = list_entry(tmp_group, struct uint_struct, uint_list) ;
      printf(" %u", group->val) ;
    }
    printf("\n") ;
  }
  
  list_for_each(tmp_lproc, &local_procs_list_head) {
    lproc = list_entry(tmp_lproc, struct localproc_struct, local_procs_list) ;
    printf("Local process: fdd_pid=%u, fdd_gid=", lproc->pid) ;
    list_for_each(tmp_gqos, &lproc->gqlist_head) {
      gqos = list_entry(tmp_gqos, struct groupqos_struct, gqlist) ;
      if(gqos->qos)
      printf("%u(TdU=%u,TmU=%u,TmrL=%u), ", gqos->gid, gqos->qos->TdU,
      gqos->qos->TmU, gqos->qos->TmrL) ;
    }
    printf("\n") ;
    
    printf("\tIs Client For:\n") ;
    list_for_each(tmp_pqos, &lproc->pqlist_head) {
      pqos = list_entry(tmp_pqos, struct procqos_struct, pqlist) ;
      printf("\t\tfdd_pid=%u on host=%u.%u.%u.%u with\n"
        "\t\tQoS=(TdU=%ums,TmU=%ums, TmrL=%us). Current status:%s\n"
        "\t\t---------\n",
        pqos->pid, NIPQUAD(&pqos->addr),
        pqos->qos.TdU, pqos->qos.TmU, pqos->qos.TmrL,
        proc_trust_list_member(lproc, pqos->pid, 0, 0, &pqos->addr)?
      "TRUST":"SUSPECT") ;
    }
    
    printf("\t All Trusted Processes:\n") ;
    list_for_each(tmp_trust, &lproc->trust_list_head) {
      trust = list_entry(tmp_trust, struct trust_struct, trust_list) ;
      if( trust->gid )
      printf("\t\tfdd_pid=%u on host=%u.%u.%u.%u in group %u. "
        "Current status: TRUST\n", trust->pid,
      NIPQUAD(&trust->host->addr), trust->gid->val) ;
    }
    
    printf("\tIs Server for:\n") ;
    list_for_each(tmp_host, &remote_host_list_head) {
      host = list_entry(tmp_host, struct host_struct, remote_host_list) ;
      if(is_in_ordered_list(lproc->pid, &host->local_servers_proc_head))
        printf("\t\tHost %u.%u.%u.%u\n", NIPQUAD(&host->addr)) ;
    }
  }
}


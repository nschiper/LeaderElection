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


/* omega_local.c */

#include "list.h"
#include "omega.h"
#include "variables_exchange.h"
#include "pipe.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "misc.h"

int omega_fifo_fd ;
struct list_head localregistered_proc_head ;

void omega_local_init() {
  INIT_LIST_HEAD(&localregistered_proc_head);
}


/* send result message type to a local process */
static int omega_send_result(struct localregistered_proc_struct *rproc, int result) {
  char msg[OMEGA_FIFO_MSG_LEN] ;
  msg_omega_build_res(msg, result) ;
  return write_msg(rproc->omega_qry_fd, msg, OMEGA_FIFO_MSG_LEN) ;
}

/* send an extended result message type to a local process */
static int omega_send_ext_result(struct localregistered_proc_struct *rproc, int result,
  struct leaders_struct *leader) {
  char msg[OMEGA_FIFO_MSG_LEN] ;
  msg_omega_build_ext_res(msg, result, &leader->addr, leader->pid, leader->stable) ;
  return write_msg(rproc->omega_qry_fd, msg, OMEGA_FIFO_MSG_LEN) ;
}


/* Removes the leader variable of a group gip if there are no more local processes in
gid. It also either deletes the contenders set of gid for the same reason as above
 or only deletes the local process from that set. */
static void cleanup_leader_and_contenders(struct localregistered_proc_struct *rproc,
  unsigned int gid, int candidate, struct timeval *now) {
  
  struct list_head *tmp_head1, *tmp_head2;
  struct contenders_struct *tmp_contenders;
  
  int local_group_empty = 1;
  
  /* boolean equal to true if the group is not empty but contains no candidate*/
  int no_candidate = 1;
  
  struct notif_type_struct *tmp_notif;
  struct localregistered_proc_struct *tmp_rproc;
  
  
  /* Check if there is another process in group gid */
  list_for_each(tmp_head1, &localregistered_proc_head) {
    tmp_rproc = list_entry(tmp_head1, struct localregistered_proc_struct, localproc_list);
    if (tmp_rproc->pid != rproc->pid) {
      list_for_each(tmp_head2, &tmp_rproc->notif_type_list) {
        tmp_notif = list_entry(tmp_head2, struct notif_type_struct, notif_type_list);
        if (tmp_notif->gid == gid) {
          local_group_empty = 0;
          if (tmp_notif->candidate == CANDIDATE) {
            no_candidate = 0;
            
          }
          break;
        }
      }
    }
    if (!no_candidate)
      break;
  }
  
  /* Remove the localLeader variable, globalLeader variable, the localContenders set and
  the globalContenders set of group gid
   if there are no more local processes in group gid. */
  if (local_group_empty) {
    int found;
    list_remove_ordered(gid, gid, &globalLeader_head, struct leaders_struct, leaders_list, <);
    list_remove_ordered(gid, gid, &localLeader_head, struct leaders_struct, leaders_list, <);
    
    list_for_each(tmp_head1, &globalContenders_head) {
      tmp_contenders = list_entry(tmp_head1, struct contenders_struct, contenders_list);
      if (tmp_contenders->gid < gid)
        continue;
      else if (tmp_contenders->gid == gid) {
        list_del(&tmp_contenders->contenders_list);
        list_free(&tmp_contenders->procs_list, struct proc_struct, proc_list);
        free(tmp_contenders);
        break;
      }
      else
        break;
    }
    
    list_for_each(tmp_head1, &localContenders_head) {
      tmp_contenders = list_entry(tmp_head1, struct contenders_struct, contenders_list);
      if (tmp_contenders->gid < gid)
        continue;
      else if (tmp_contenders->gid == gid) {
        list_del(&tmp_contenders->contenders_list);
        list_free(&tmp_contenders->procs_list, struct proc_struct, proc_list);
        free(tmp_contenders);
        break;
      }
      else
        break;
    }
  }
  else if (no_candidate) {
    int found;
    list_remove_ordered(gid, gid, &localLeader_head, struct leaders_struct, leaders_list, <);
    
    /* Remove the process from the localContenders set. If the process
     is not a candidate, nothing will be done.*/
    remove_proc_from_localContenders_set(&omega_localaddr, rproc->pid, gid);
    
    /* Remove the process from the globalContenders set. If the process
     is not a candidate, nothing will be done.*/
    remove_proc_from_globalContenders_set(&omega_localaddr, rproc->pid, gid);
    
    if (updateGlobalLeader(gid, now) < 0)
    fprintf(stderr, "omega: Error updating the global leader of group: %u, called from"
    "cleanup_leaders_and_contenders", gid);
  }
  
  /* If the local group was empty we would already have deleted the contenders set, so there
   would be no point in deleting the process from that set. */
  else {
    /* If the process that just left the group was not a candidate, do nothing.*/
    if (candidate == CANDIDATE) {
      if (remove_proc_from_localContenders_set(&omega_localaddr, rproc->pid, gid) < 0)
      fprintf(stderr, "omega: cleanup_leader_contenders, Error when removing proc: %u from contenders set of group:%u\n",
      rproc->pid, gid);
      if (updateLocalLeader(gid, now) < 0)
      fprintf(stderr, "omega: Error in updateLocalLeader for group: %u called from cleanup_leader_and_contenders\n",
      gid);
    }
  }
}



/* register a local process */
static void omega_local_reg(char *msg, fd_set *dset, struct timeval *now) {
  struct localregistered_proc_struct *entry_ptr ;
  
  char fifo_name[OMEGA_FIFO_MSG_LEN] ;
  unsigned int pid ;
  int omega_cmd_fd, omega_qry_fd, omega_int_fd;
  int retval = 0 ;
  
  msg_omega_parse_reg(msg, &pid) ;
  
  
  /* Register process in the fd service */
  if (fdd_local_reg(pid, now) < 0)
    goto error;
  
  entry_ptr = malloc(sizeof(*entry_ptr));
  
  if (entry_ptr == NULL)
    goto error;
  
  /* app must open these O_RDONLY | O_NONBLOCK before registering */
  snprintf(fifo_name, OMEGA_FIFO_MSG_LEN, OMEGA_FIFO_QRY, pid);
  omega_qry_fd = open(fifo_name, O_WRONLY | O_NONBLOCK);
  if(omega_qry_fd == -1) {
    retval = -errno ;
    
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "Could not open fifo\n") ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "Could not open fifo\n") ;
#endif
    
    goto error_free ;
  }
  
  snprintf(fifo_name, OMEGA_FIFO_MSG_LEN, OMEGA_FIFO_INT, pid);
  omega_int_fd = open(fifo_name, O_WRONLY | O_NONBLOCK);
  if (omega_int_fd == -1) {
    retval = -errno;
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "Could not open fifo\n") ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "Could not open fifo\n") ;
#endif
    goto error_close_qry;
  }
  
  /* app must open this O_WRONLY after registering, *
   * it will block until we open it here            */
  snprintf(fifo_name, OMEGA_FIFO_MSG_LEN, OMEGA_FIFO_CMD, pid) ;
  omega_cmd_fd = open(fifo_name, O_RDONLY | O_NONBLOCK) ;
  if(omega_cmd_fd == -1) {
    retval = -errno ;
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "Could not open fifo\n") ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "Could not open fifo\n") ;
#endif
    goto error_close_int ;
  }
  
  entry_ptr->omega_cmd_fd = omega_cmd_fd ;
  entry_ptr->omega_qry_fd = omega_qry_fd ;
  entry_ptr->omega_int_fd = omega_int_fd ;
  entry_ptr->pid = pid;
  INIT_LIST_HEAD(&entry_ptr->notif_type_list);
  
  list_add_tail(&(entry_ptr->localproc_list), &localregistered_proc_head);
  retval = omega_send_result(entry_ptr, retval) ;
  
  
  if (retval < 0) {
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "Could not send results\n") ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "Could not send results\n") ;
#endif
    goto error_close_int ;
  }
  
  FD_SET(omega_cmd_fd, dset);
  
#ifdef OMEGA_OUTPUT
  fprintf(stdout, "LocalProc %u Registered in Omega\n", pid);
#endif
#ifdef OMEGA_LOG
  fprintf(omega_log, "LocalProc %u Registered in Omega\n", pid);
#endif
  
  return ;
  
  error_close_int:
  close(omega_int_fd);
  error_close_qry:
  close(omega_qry_fd);
  error_free:
  free(entry_ptr);
  error:
#ifdef OMEGA_OUTPUT
  fprintf(stderr, "omega: omega_local_reg failed %d\n", retval) ;
#endif
#ifdef OMEGA_LOG
  fprintf(omega_log, "omega: omega_local_reg failed %d\n", retval) ;
#endif
  return;
}



/* unregister a local process */
static void omega_local_unreg(struct localregistered_proc_struct *rproc, fd_set *dset, struct timeval *now) {
  
  struct list_head *tmp_head;
  struct notif_type_struct *tmp_notif;
  int candidate;
  int gid;
  
  if (fdd_local_unreg(rproc->pid, now) < 0) {
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "omega: omega_local_unreg failed\n") ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "omega: omega_local_unreg failed\n") ;
#endif
  }
  
  
  list_del(&rproc->localproc_list);
  
  /* Make sure that the process unregistering is not in a set of actives.
   He could have died and so it didn't execute stopOmega. */
  list_for_each(tmp_head, &rproc->notif_type_list) {
    tmp_notif = list_entry(tmp_head, struct notif_type_struct, notif_type_list);
    tmp_head = tmp_head->prev;
    candidate = tmp_notif->candidate;
    gid = tmp_notif->gid;
    /* Delete the notif_type for the group gid, to be sure
    that the process unregistering will not be notified of
     the eventual leader change. */
    list_del(&tmp_notif->notif_type_list);
    free(tmp_notif);
    cleanup_leader_and_contenders(rproc, gid, candidate, now);
  }
  
  
  close(rproc->omega_cmd_fd);
  close(rproc->omega_qry_fd);
  close(rproc->omega_int_fd);
  
  FD_CLR(rproc->omega_cmd_fd, dset);
  free(rproc) ;
}


static void do_startOmega(struct localregistered_proc_struct *rproc, char *msg, struct timeval *now) {
  
  unsigned int gid ;
  int candidate ;
  int notif_type;
  unsigned int TdU ;
  unsigned int TmU ;
  unsigned int TmrL ;
  
  int retval = 0 ;
  
  msg_omega_parse_startomega(msg, &gid, &candidate, &notif_type, &TdU, &TmU, &TmrL);
  
  if (notif_type != OMEGA_INTERRUPT_ANY_CHANGE &&
    notif_type != OMEGA_INTERRUPT_NONE) {
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "omega: do_start_omega, notif_type parameter incorrect\n") ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "omega: do_start_omega, notif_type parameter incorrect\n") ;
#endif
    retval = -1;
    goto out;
  }
  else {
    struct notif_type_struct *entry_ptr = NULL;
    int entry_exists;
    
    list_insert_ordered(gid, gid, &rproc->notif_type_list, struct notif_type_struct,
    notif_type_list, <);
    if (entry_ptr == NULL) {
      retval = -ENOMEM;
      goto out;
    }
    else {
      entry_ptr->notif_type = notif_type;
      entry_ptr->already_notified = 0;
      
      if (candidate == CANDIDATE)
        entry_ptr->candidate = CANDIDATE;
      else
        entry_ptr->candidate = NOT_CANDIDATE;
    }
  }
  
  if (candidate == CANDIDATE) {
    if (do_join_group_invisible(rproc->pid, gid, candidate, now) < 0) {
#ifdef OMEGA_OUTPUT
      fprintf(stderr, "omega: join group invisible: %u failed\n", gid) ;
#endif
#ifdef OMEGA_LOG
      fprintf(omega_log, "omega: join group invisible: %u failed\n", gid) ;
#endif
      retval = -1;
      goto out;
    }
    if (do_monitor_group(rproc->pid, gid, INTERRUPT_ANY_CHANGE, TdU, TmU, TmrL, now) < 0) {
#ifdef OMEGA_OUTPUT
      fprintf(stderr, "omega: monitor group: %u failed\n", gid) ;
#endif
#ifdef OMEGA_LOG
      fprintf(omega_log, "omega: monitor group: %u failed\n", gid) ;
#endif
      retval = -1;
      goto out;
    }
    
    
    /* Add the process in the contenders set */
    if (add_proc_in_localContenders_set(&omega_localaddr, rproc->pid, gid) < 0)
    fprintf(stderr, "doStartOmega: Error while adding local proc: %u in contenders set of group: %u\n",
    rproc->pid, gid);
    /* update the leader of group gid */
    if (updateLocalLeader(gid, now) < 0)
    fprintf(stderr, "omega: Error in updateLocalLeader for group: %u called from startOmega\n",
    gid);
  }
  else if (candidate == NOT_CANDIDATE) {
    if (do_join_group_invisible(rproc->pid, gid, candidate, now) < 0) {
#ifdef OMEGA_OUTPUT
      fprintf(stderr, "omega: join group invisible: %u failed\n", gid) ;
#endif
#ifdef OMEGA_LOG
      fprintf(omega_log, "omega: join group invisible: %u failed\n", gid) ;
#endif
      retval = -1;
      goto out;
    }
    if (do_monitor_group(rproc->pid, gid, INTERRUPT_ANY_CHANGE, TdU, TmU, TmrL, now) < 0) {
#ifdef OMEGA_OUTPUT
      fprintf(stderr, "omega: monitor group: %u failed\n", gid) ;
#endif
#ifdef OMEGA_LOG
      fprintf(omega_log, "omega: monitor group: %u failed\n", gid) ;
#endif
      retval = -1;
      goto out;
    }
    if (updateGlobalLeader(gid, now) < 0)
    fprintf(stderr, "omega: Error executing updateGlobalLeader of group: %u"
    " called from do_startOmega\n", gid);
  }
  else {
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "omega: do_start_omega, candidate parameter incorrect\n") ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "omega: do_start_omega, candidate parameter incorrect\n") ;
#endif
    retval = -1;
    goto out;
  }
  
  
  out:
  retval = omega_send_result(rproc, retval) ;
  if(retval < 0) {
#ifdef OMEGA_OUTPUT
    fprintf(stdout, "Error in do_start_omega\n") ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "Error in do_start_omega\n") ;
#endif
  }
}


static void do_stopOmega(struct localregistered_proc_struct *rproc, char *msg, struct timeval *now) {
  
  unsigned int gid ;
  int retval = 0 ;
  int candidate = NOT_CANDIDATE;
  struct list_head *tmp_head;
  struct notif_type_struct *tmp_notif;
  
  msg_omega_parse_stopomega(msg, &gid);
  
  if (do_stop_monitor_group(rproc->pid, gid, now) < 0) {
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "omega: stop monitor group: %u failed\n", gid) ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "omega: stop monitor group: %u failed\n", gid) ;
#endif
    retval = -1;
    goto out;
  }
  
  if (do_leave_group(rproc->pid, gid, now) < 0) {
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "omega: leave group: %u failed\n", gid) ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "omega: leave group: %u failed\n", gid) ;
#endif
    retval = -1;
    goto out;
  }
  
  
  /* remove the notif_type of group gid to be sure the process stopping
   omega will not be notified of an eventual leader change. */
  list_for_each(tmp_head, &rproc->notif_type_list) {
    tmp_notif = list_entry(tmp_head, struct notif_type_struct, notif_type_list);
    if (tmp_notif->gid == gid) {
      candidate = tmp_notif->candidate;
      list_del(&tmp_notif->notif_type_list);
      free(tmp_notif);
      break;
    }
  }
  
  /* Remove the process from the contenders of group gid. */
  cleanup_leader_and_contenders(rproc, gid, candidate, now);
  
  out:
  retval = omega_send_result(rproc, retval) ;
  if(retval < 0) {
#ifdef OMEGA_OUTPUT
    fprintf(stdout, "Error in do_stop_omega\n") ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "Error in do_stop_omega\n") ;
#endif
  }
}


static void do_change_interrupt_mode(struct localregistered_proc_struct *rproc, char *msg) {
  
  unsigned int gid;
  int notif_type;
  int retval = 0;
  
  msg_omega_parse_int(msg, &notif_type, &gid);
  
  {
    struct notif_type_struct *entry_ptr = NULL;
    int entry_exists;
    
    list_insert_ordered(gid, gid, &rproc->notif_type_list, struct notif_type_struct,
    notif_type_list, <);
    if (!entry_exists) {
      retval = -1;
      goto out;
    }
    else {
      entry_ptr->notif_type = notif_type;
      if (notif_type == OMEGA_INTERRUPT_NONE)
        entry_ptr->already_notified = 0;
    }
  }
  
  out:
  retval = omega_send_result(rproc, retval) ;
  if(retval < 0) {
#ifdef OMEGA_OUTPUT
    fprintf(stdout, "Error in do_change_interrupt_mode\n") ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "Error in do_change_interrupt_mode\n") ;
#endif
  }
}


static void do_get_leader(struct localregistered_proc_struct *rproc, char *msg) {
  
  struct list_head *tmp_head;
  struct leaders_struct *leader, leader_cpy;
  int leader_found = 0, retval = 0;
  unsigned int gid;
  
  
  msg_omega_parse_getleader(msg, &gid);
  
  list_for_each(tmp_head, &globalLeader_head) {
    leader = list_entry(tmp_head, struct leaders_struct, leaders_list);
    if (leader->gid == gid) {
      leader_found = 1;
      leader_cpy.gid = leader->gid;
      memcpy(&(leader_cpy.addr), &leader->addr, sizeof(struct sockaddr_in));
      leader_cpy.pid = leader->pid;
      leader_cpy.stable = leader->stable;
      break;
    }
  }
  
  if (!leader_found)
    retval = -1;
  
  retval = omega_send_ext_result(rproc, retval, &leader_cpy) ;
  if(retval < 0) {
#ifdef OMEGA_OUTPUT
    fprintf(stdout, "omega: Error while writing in the query pipe in do_get_leader \n") ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "omega: Error while writing in the query pipe in do_get_leader\n") ;
#endif
  }
}


/* execute a command received from a local process by the command pipe */
static void omega_do_cmd(struct localregistered_proc_struct *rproc, fd_set *dset, struct timeval *now) {
  char msg[OMEGA_FIFO_MSG_LEN] ;
  ssize_t len ;
  
  len = read(rproc->omega_cmd_fd, msg, OMEGA_FIFO_MSG_LEN);
  if(len == -1) {
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "omega: reading from cmd fifo failed:%s\n", strerror(errno)) ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "omega: reading from cmd fifo failed:%s\n", strerror(errno)) ;
#endif
    goto out;
  }
  
  if (len != 0 && len < OMEGA_FIFO_MSG_LEN) {
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "omega: not enough data in cmd fifo:%s\n", strerror(errno)) ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "omega: not enough data in cmd fifo:%s\n", strerror(errno)) ;
#endif
    goto out;
  }
  
  if (len == 0) { /* pipe was broked. unregister the local process */
#ifdef OMEGA_OUTPUT
    fprintf(stdout, "OMEGA DEATH_LOCAL_PROC(%u)\n", rproc->pid) ;
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "OMEGA DEATH_LOCAL_PROC(%u)\n", rproc->pid) ;
#endif
    omega_local_unreg(rproc, dset, now) ;
    goto out;
  }
  
  switch (msg_type(msg)) {
    case MSG_OMEGA_STARTOMEGA:
      do_startOmega(rproc, msg, now);
    break;
    case MSG_OMEGA_STOPOMEGA:
      do_stopOmega(rproc, msg, now);
    break;
    
    case MSG_OMEGA_GET_LEADER:
      do_get_leader(rproc, msg);
    break;
    
    case MSG_OMEGA_INTERRUPT_MODE:
      do_change_interrupt_mode(rproc, msg);
    break;
    
    default:
#ifdef OMEGA_OUTPUT
    fprintf(stderr, "omega: bad message type on cmd fifo %d.\n", msg_type(msg));
#endif
#ifdef OMEGA_LOG
    fprintf(omega_log, "omega: bad message type on cmd fifo %d.\n", msg_type(msg));
#endif
    break;
  }
  out:
  return ;
}


extern void omega_local_check_pipes(fd_set *active, fd_set *dset, struct timeval *now) {
  struct list_head *tmp          = NULL ;
  struct list_head *head         = NULL;
  struct localregistered_proc_struct *rproc = NULL ;
  
  head = &localregistered_proc_head ;
  for( tmp = head->next ; tmp != head ; ) {
    rproc = list_entry(tmp, struct localregistered_proc_struct, localproc_list);
    tmp = tmp->next ;
    if( FD_ISSET(rproc->omega_cmd_fd, active) ) {
      omega_do_cmd(rproc, dset, now) ;
    }
  }
}



/* check_fifo - reads a registration message on the registration fifo */
extern void check_omega_fifo(fd_set *active, fd_set *dset, struct timeval *now) {
  char msg[OMEGA_FIFO_MSG_LEN] ;
  ssize_t len ;
  
  if(!FD_ISSET(omega_fifo_fd, active))
    goto out ;
  
  len = read(omega_fifo_fd, msg, OMEGA_FIFO_MSG_LEN) ;
  if(len == -1) {
    fprintf(stderr, "omega: reading from omega_fifo failed\n") ;
    goto out ;
  }
  
  if(len != 0 && len < OMEGA_FIFO_MSG_LEN) {
    fprintf(stderr, "omega: not enough data in omega_fifo\n") ;
    goto out ;
  }
  
  if(len == 0) {
    if (omega_fifo_reinit() < 0)
      fprintf(stderr, "omega: could not reopen omega_fifo!\n");
    goto out;
  }
  
  switch(msg_type(msg)) {
    case MSG_OMEGA_REGISTER:
      omega_local_reg(msg, dset, now);
    break;
    default:
      fprintf(stderr, "omega: bad message type on omega_fifo %d.\n", msg_type(msg));
    break;
  }
  out:
  return;
}


/* Checks if the group exists locally. If the candidate parameter is true,
 the group has to exist and contain local candidates. */
extern int omega_group_exists_locally(unsigned int gid, int candidate) {
  
  struct list_head *tmp_rproc, *tmp_notif;
  struct localregistered_proc_struct *rproc;
  struct notif_type_struct *notif;
  
  list_for_each(tmp_rproc, &localregistered_proc_head) {
    rproc = list_entry(tmp_rproc, struct localregistered_proc_struct, localproc_list);
    list_for_each(tmp_notif, &rproc->notif_type_list) {
      notif = list_entry(tmp_notif, struct notif_type_struct, notif_type_list);
      if (notif->gid == gid) {
        if (((candidate == CANDIDATE) && (notif->candidate == CANDIDATE)) ||
        (candidate == NOT_CANDIDATE))
        return 1;
      }
    }
  }
  return 0;
}

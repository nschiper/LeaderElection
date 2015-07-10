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


/* omega_algorithm.c */

#include "omega.h"
#include "misc.h"
#include "variables_exchange.h"
#include "pipe.h"
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/time.h>


/* The list of local leaders (one per group) */
struct list_head localLeader_head;

/* The list of list of local contenders (one list per group) */
struct list_head localContenders_head;

/* The list of global leaders (one per group) */
struct list_head globalLeader_head;

/* The list of list of global contenders (one list per group) */
struct list_head globalContenders_head;


int omega_algorithm_init() {
  
  INIT_LIST_HEAD(&(localLeader_head));
  INIT_LIST_HEAD(&(localContenders_head));
  INIT_LIST_HEAD(&(globalLeader_head));
  INIT_LIST_HEAD(&(globalContenders_head));
  
  return 0;
}

/* Returns 1 if addr is a local address, 0 otherwise */
int is_local_address(struct sockaddr_in *addr) {
  return sockaddr_eq(addr, &omega_localaddr);
}


inline int add_proc_in_localContenders_set(struct sockaddr_in *addr, u_int pid, u_int gid) {
  
  struct contenders_struct *entry_ptr = NULL;
  int entry_exists;
  struct proc_struct *tmp_proc;
  struct list_head *tmp_head;
  int found_proc = 0;
  
  list_insert_ordered(gid, gid, &localContenders_head, struct contenders_struct, contenders_list, <);
  
  if (entry_ptr == NULL)
    return -1;
  else if (!entry_exists)
    INIT_LIST_HEAD(&entry_ptr->procs_list);
  
  list_for_each(tmp_head, &entry_ptr->procs_list) {
    tmp_proc = list_entry(tmp_head, struct proc_struct, proc_list);
    if ((tmp_proc->pid == pid) && sockaddr_eq(addr, &tmp_proc->addr)) {
      found_proc = 1;
      break;
    }
  }
  if (!found_proc) {
    tmp_proc = malloc(sizeof(*tmp_proc));
    if (tmp_proc == NULL)
      return -1;
    else {
      tmp_proc->pid = pid;
      memcpy(&tmp_proc->addr, addr, sizeof(struct sockaddr_in));
      list_add_tail(&(tmp_proc->proc_list), tmp_head);
    }
  }
  return 0;
}

inline int remove_proc_from_localContenders_set(struct sockaddr_in *addr, u_int pid, u_int gid) {
  
  struct list_head *tmp_head1, *tmp_head2;
  struct contenders_struct *tmp_contenders;
  struct proc_struct *tmp_proc;
  
  list_for_each(tmp_head1, &localContenders_head) {
    tmp_contenders = list_entry(tmp_head1, struct contenders_struct, contenders_list);
    if (tmp_contenders->gid < gid)
      continue;
    else if (tmp_contenders->gid == gid) {
      list_for_each(tmp_head2, &tmp_contenders->procs_list) {
        tmp_proc = list_entry(tmp_head2, struct proc_struct, proc_list);
        if (sockaddr_eq(&tmp_proc->addr, addr) &&
          (pid == tmp_proc->pid)) {
          list_del(&tmp_proc->proc_list);
          free(tmp_proc);
          break;
        }
      }
      break;
    }
    else
      break;
  }
  return 0;
}



inline int add_or_replace_locaLeader_in_globalContenders_set(u_int pid, u_int gid) {
  
  struct contenders_struct *entry_ptr = NULL;
  int entry_exists;
  struct proc_struct *tmp_proc;
  struct list_head *tmp_head;
  int found_proc = 0;
  
  list_insert_ordered(gid, gid, &globalContenders_head, struct contenders_struct, contenders_list, <);
  
  if (entry_ptr == NULL)
    return -1;
  else if (!entry_exists)
    INIT_LIST_HEAD(&entry_ptr->procs_list);
  
  list_for_each(tmp_head, &entry_ptr->procs_list) {
    tmp_proc = list_entry(tmp_head, struct proc_struct, proc_list);
    if (is_local_address(&tmp_proc->addr)) {
      found_proc = 1;
      tmp_proc->pid = pid;
      break;
    }
  }
  if (!found_proc) {
    tmp_proc = malloc(sizeof(*tmp_proc));
    if (tmp_proc == NULL)
      return -1;
    else {
      tmp_proc->pid = pid;
      memcpy(&tmp_proc->addr, &omega_localaddr, sizeof(struct sockaddr_in));
      list_add_tail(&(tmp_proc->proc_list), &entry_ptr->procs_list);
    }
  }
  return 0;
}


inline int add_proc_in_globalContenders_set(struct sockaddr_in *addr, u_int pid, u_int gid) {
  
  struct contenders_struct *entry_ptr = NULL;
  int entry_exists;
  struct proc_struct *tmp_proc;
  struct list_head *tmp_head;
  int found_proc = 0;
  
  list_insert_ordered(gid, gid, &globalContenders_head, struct contenders_struct, contenders_list, <);
  
  if (entry_ptr == NULL)
    return -1;
  else if (!entry_exists)
    INIT_LIST_HEAD(&entry_ptr->procs_list);
  
  list_for_each(tmp_head, &entry_ptr->procs_list) {
    tmp_proc = list_entry(tmp_head, struct proc_struct, proc_list);
    if ((tmp_proc->pid == pid) && sockaddr_eq(addr, &tmp_proc->addr)) {
      found_proc = 1;
      break;
    }
  }
  if (!found_proc) {
    tmp_proc = malloc(sizeof(*tmp_proc));
    if (tmp_proc == NULL)
      return -1;
    else {
      tmp_proc->pid = pid;
      memcpy(&tmp_proc->addr, addr, sizeof(struct sockaddr_in));
      list_add_tail(&(tmp_proc->proc_list), &entry_ptr->procs_list);
    }
  }
  return 0;
}

inline int remove_proc_from_globalContenders_set(struct sockaddr_in *addr, u_int pid, u_int gid) {
  
  struct list_head *tmp_head1, *tmp_head2;
  struct contenders_struct *tmp_contenders;
  struct proc_struct *tmp_proc;
  
  list_for_each(tmp_head1, &globalContenders_head) {
    tmp_contenders = list_entry(tmp_head1, struct contenders_struct, contenders_list);
    if (tmp_contenders->gid < gid)
      continue;
    else if (tmp_contenders->gid == gid) {
      list_for_each(tmp_head2, &tmp_contenders->procs_list) {
        tmp_proc = list_entry(tmp_head2, struct proc_struct, proc_list);
        if (sockaddr_eq(&tmp_proc->addr, addr) &&
          (pid == tmp_proc->pid)) {
          list_del(&tmp_proc->proc_list);
          free(tmp_proc);
          return 0;
        }
      }
      break;
    }
    else
      break;
  }
  return -1;
}


/* Returns 1 if the proc is "smaller" than the current temporary leader, 0 otherwise.
To determine which one is the smallest, we proceed in the following way:
1) compare their accusationTime variable, if they are equal goto 2)
 2) compare their IP address. */
static inline int compare_procs(struct leaders_struct *temp_newleader, struct timeval *newleader_accusationTime,
  struct proc_struct *proc, struct timeval *proc_accusationTime) {
  if (timercmp(proc_accusationTime, newleader_accusationTime, <))
    return 1;
  else if (timercmp(proc_accusationTime, newleader_accusationTime, ==)) {
    if (sockaddr_smaller(&proc->addr, &temp_newleader->addr))
      return 1;
    else
      return 0;
  }
  else
    return 0;
}


inline int updateLocalLeader(unsigned int gid, struct timeval *now) {
  struct leaders_struct *localLeader = NULL, *newLocalLeader;
  struct contenders_struct *tmp_localContenders;
  struct proc_struct *tmp_proc;
  struct list_head *tmp_head1, *tmp_head2;
  int localLeader_found = 0;
  struct notif_type_struct *tmp_notif;
  struct localregistered_proc_struct *rproc;
  struct leaders_struct *globalLeader = NULL;
  char msg[OMEGA_FIFO_MSG_LEN];
  
  
  /* Update the leader for group gid */
  /* Get the current leader or create it if it doesn't exist */
  list_for_each(tmp_head1, &localLeader_head) {
    localLeader = list_entry(tmp_head1, struct leaders_struct, leaders_list);
    if (localLeader->gid < gid)
      continue;
    else if (localLeader->gid == gid) {
      localLeader_found = 1;
      break;
    }
    else /* No localLeaders yet for group gid */
      break;
  }
  if (!localLeader_found) {
    localLeader = malloc(sizeof(*localLeader));
    
    if (localLeader == NULL)
      return -1;
    else {
      localLeader->gid = gid;
      localLeader->pid = 0; /* localLeader address and pid not yet assigned */
      list_add_tail(&(localLeader->leaders_list), tmp_head1);
    }
  }
  
  newLocalLeader = malloc(sizeof(*newLocalLeader));
  newLocalLeader->gid = gid;
  memcpy(&newLocalLeader->addr, &omega_localaddr, sizeof(struct sockaddr_in));
  newLocalLeader->pid = 0; /* newLocalLeader pid not yet assigned */
  
  list_for_each(tmp_head1, &localContenders_head) {
    tmp_localContenders = list_entry(tmp_head1, struct contenders_struct, contenders_list);
    if (tmp_localContenders->gid < gid)
      continue;
    
    /* The newLocalLeader is the first process of the contenders set. */
    else if (tmp_localContenders->gid == gid) {
      list_for_each(tmp_head2, &tmp_localContenders->procs_list) {
        tmp_proc = list_entry(tmp_head2, struct proc_struct, proc_list);
        newLocalLeader->pid = tmp_proc->pid;
        break;
      }
      break;
    }
  }
  
  if (localLeader->pid != newLocalLeader->pid) {
    if (add_or_replace_locaLeader_in_globalContenders_set(newLocalLeader->pid, gid) < 0)
      fprintf(stderr, "updateLocalLeader: error while adding or replacing localLeader in globalContenders\n");
    
    if (updateGlobalLeader(gid, now) < 0) {
      free(newLocalLeader);
      return -1;
    }
    localLeader->pid = newLocalLeader->pid;
  }
  else { /* Send the globalLeader to all the local processes that haven't been notified yet. */
    
    list_for_each(tmp_head1, &globalLeader_head) {
      globalLeader = list_entry(tmp_head1, struct leaders_struct, leaders_list);
      if (globalLeader->gid < gid)
        continue;
      else if (globalLeader->gid == gid)
        break;
      else {
        fprintf(stderr, "omega updateLocalLeader: error couldn't find actual globalLeader for group: %u\n",
        gid);
        break;
      }
    }
    
    list_for_each(tmp_head1, &localregistered_proc_head) {
      rproc = list_entry(tmp_head1, struct localregistered_proc_struct, localproc_list);
      list_for_each(tmp_head2, &rproc->notif_type_list) {
        tmp_notif = list_entry(tmp_head2, struct notif_type_struct, notif_type_list);
        if ((tmp_notif->notif_type == OMEGA_INTERRUPT_ANY_CHANGE) &&
          (!tmp_notif->already_notified)) {
          msg_omega_build_notify(msg, &globalLeader->addr, globalLeader->pid, globalLeader->gid,
          globalLeader->stable);
          
          if (write_msg(rproc->omega_int_fd, msg, OMEGA_FIFO_MSG_LEN) < 0)
            fprintf(stderr, "omega updateLocalLeader: error while writing to interrupt pipe\n");
          else
            tmp_notif->already_notified = 1;
        }
      }
    }
  }
  free(newLocalLeader);
  return 0;
}

int updateGlobalLeaders(struct timeval *now) {
  
  struct contenders_struct *tmp_contenders;
  struct list_head *tmp_head;
  
  list_for_each(tmp_head, &globalContenders_head) {
    tmp_contenders = list_entry(tmp_head, struct contenders_struct, contenders_list);
    if (updateGlobalLeader(tmp_contenders->gid, now) < 0)
      return -1;
  }
  return 0;
}

inline int updateGlobalLeader(unsigned int gid, struct timeval *now) {
  
  struct leaders_struct *globalLeader = NULL, *newGlobalLeader, *tmp_leader;
  struct contenders_struct *tmp_globalContenders;
  struct proc_struct *tmp_proc;
  struct list_head *tmp_head1, *tmp_head2, *tmp_head3;
  struct timeval accusationTime1, accusationTime2;
  int globalLeader_found = 0;
  struct localregistered_proc_struct *rproc;
  struct notif_type_struct *tmp_notif;
  char msg[OMEGA_FIFO_MSG_LEN];
  int nb_remote_proc_in_contenders = 0;
  int addr_changed, pid_changed, stability_changed,
  leader_changed;
  
  
  /* Update the leader for group gid */
  /* Get the current leader or create it if it doesn't exist */
  list_for_each(tmp_head1, &globalLeader_head) {
    globalLeader = list_entry(tmp_head1, struct leaders_struct, leaders_list);
    if (globalLeader->gid < gid)
      continue;
    else if (globalLeader->gid == gid) {
      globalLeader_found = 1;
      break;
    }
    else /* No globalLeaders yet for group gid */
      break;
  }
  if (!globalLeader_found) {
    globalLeader = malloc(sizeof(*globalLeader));
    
    if (globalLeader == NULL) {
      fprintf(stderr, "updateGlobalLeader: error couldn't allocate memory for globalLeader of group: %u\n",
      gid);
      return -1;
    }
    else {
      globalLeader->gid = gid;
      globalLeader->pid = 0; /* leader address and pid not yet assigned */
      globalLeader->stable = 0;
      memset(&globalLeader->addr, 0, sizeof(struct sockaddr_in));
      list_add_tail(&(globalLeader->leaders_list), tmp_head1);
    }
  }
  
  newGlobalLeader = malloc(sizeof(*newGlobalLeader));
  newGlobalLeader->gid = gid;
  newGlobalLeader->pid = 0; /* leader address and pid not yet assigned */
  memset(&newGlobalLeader->addr, 0x0, sizeof(struct sockaddr_in));
  
  /* Get the newGlobalLeader */
  list_for_each(tmp_head2, &globalContenders_head) {
    tmp_globalContenders = list_entry(tmp_head2, struct contenders_struct, contenders_list);
    if (tmp_globalContenders->gid < gid)
      continue;
    else if (tmp_globalContenders->gid == gid) {
      
      list_for_each(tmp_head3, &tmp_globalContenders->procs_list) {
        struct timeval startT, accT;
        
        tmp_proc = list_entry(tmp_head3, struct proc_struct, proc_list);
        
        if (is_local_address(&tmp_proc->addr))
          getlocalvars(gid, &accT, &startT);
        else {
          get_accusationTime_of_remoteprocess(&tmp_proc->addr, gid, &accT);
          get_startTime_of_remoteprocess(&tmp_proc->addr, gid, &startT);
        }
      }
      
      list_for_each(tmp_head3, &tmp_globalContenders->procs_list) {
        tmp_proc = list_entry(tmp_head3, struct proc_struct, proc_list);
        /* The process is local */
        if (is_local_address(&tmp_proc->addr)) {
          if (getlocalaccusationTime(gid, &accusationTime1) < 0) {
            free(newGlobalLeader);
            fprintf(stderr, "updateGlobalLeader: error couldn't find local accusationTime for group: %u\n", gid);
            return -1;
          }
          else {
            if (newGlobalLeader->pid == 0) { /* temporary new leader not yet initialized */
              newGlobalLeader->pid = tmp_proc->pid;
              memcpy(&newGlobalLeader->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
            }
            else if (is_local_address(&newGlobalLeader->addr)) { /* temporary new global leader is local */
              if (getlocalaccusationTime(gid, &accusationTime2) < 0) {
                fprintf(stderr, "updateGlobalLeader: error couldn't find local accusationTime for gid: %u\n", gid);
                free(newGlobalLeader);
                return -1;
              }
              else {
                if(compare_procs(newGlobalLeader, &accusationTime2, tmp_proc, &accusationTime1)) {
                  newGlobalLeader->pid = tmp_proc->pid;
                  memcpy(&newGlobalLeader->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
                }
              }
            }
            else { /* temporary new global leader is remote */
              if (get_accusationTime_of_remoteprocess(&newGlobalLeader->addr, gid, &accusationTime2) < 0) {
                fprintf(stderr, "updateGlobalLeader: error couldn't find remote accusationTimeu for group: %u and addr: %u.%u.%u.%u\n",
                gid, NIPQUAD(&newGlobalLeader->addr));
                free(newGlobalLeader);
                return -1;
              }
              else {
                if(compare_procs(newGlobalLeader, &accusationTime2, tmp_proc, &accusationTime1)) {
                  newGlobalLeader->pid = tmp_proc->pid;
                  memcpy(&newGlobalLeader->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
                }
              }
            }
          }
        }
        /* The process is remote */
        else {
          nb_remote_proc_in_contenders++;
          if (get_accusationTime_of_remoteprocess(&tmp_proc->addr, gid, &accusationTime1) < 0) {
            fprintf(stderr, "updateGlobalLeader: error couldn't find remote accusationTime for group: %u and addr: %u.%u.%u.%u\n",
            gid, NIPQUAD(&tmp_proc->addr));
            free(newGlobalLeader);
            return -1;
          }
          else {
            if (newGlobalLeader->pid == 0) { /* temporary new global leader not yet initialized */
              newGlobalLeader->pid = tmp_proc->pid;
              memcpy(&newGlobalLeader->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
            }
            else if (is_local_address(&newGlobalLeader->addr)) { /* temporary new global leader is local */
              if (getlocalaccusationTime(gid, &accusationTime2) < 0) {
                fprintf(stderr, "updateGlobalLeader: error couldn't find local counter of fo group: %u\n", gid);
                free(newGlobalLeader);
                return -1;
              }
              else {
                if(compare_procs(newGlobalLeader, &accusationTime2, tmp_proc, &accusationTime1)) {
                  newGlobalLeader->pid = tmp_proc->pid;
                  memcpy(&newGlobalLeader->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
                }
              }
            }
            else { /* temporary new global leader is remote */
              if (get_accusationTime_of_remoteprocess(&newGlobalLeader->addr, gid, &accusationTime2) < 0) {
                fprintf(stderr, "updateGlobalLeader: error couldn't find remote accusationTime for group: %u and addr: %u.%u.%u.%u\n",
                gid, NIPQUAD(&newGlobalLeader->addr));
                free(newGlobalLeader);
                return -1;
              }
              else {
                if(compare_procs(newGlobalLeader, &accusationTime2, tmp_proc, &accusationTime1)) {
                  newGlobalLeader->pid = tmp_proc->pid;
                  memcpy(&newGlobalLeader->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
                }
              }
            }
          }
        }
      }
      break;
    }
    else
      fprintf(stderr, "omega updateGlobalLeader: error couldn't find contenders set for group: %u\n", gid);
  }
  
  if ((!sockaddr_eq(&globalLeader->addr, &newGlobalLeader->addr)) ||
    (globalLeader->pid != newGlobalLeader->pid)) {
    
    if (is_local_address(&newGlobalLeader->addr)) {  /* If a local process gains the leadership */
      if (set_local_startTime(gid) < 0)
        fprintf(stderr, "omega updateGlobalLeader: error couldn't set startTime for group: %u\n", gid);
      
      if (do_restart_sending_alives(newGlobalLeader->pid, gid, now) < 0) {
        fprintf(stderr, "updateGlobalLeader: error couldn't restart sending alives for proc: %u group: %u\n",
        newGlobalLeader->pid, gid);
        free(newGlobalLeader);
        return -1;
      }
    }
    if ((!sockaddr_eq(&globalLeader->addr, &newGlobalLeader->addr)) &&
      is_local_address(&globalLeader->addr)) { /* If a local process loses the leadership */
      
      if (isalive(globalLeader->pid)) {
        if (do_stop_sending_alives(globalLeader->pid, gid, now) < 0) {
          free(newGlobalLeader);
          fprintf(stderr, "updateGlobalLeader: error couldn't stop sending alives for proc: %u group: %u\n",
          globalLeader->pid, gid);
          return -1;
        }
        
        if (set_local_startTime(gid) < 0) {
          free(newGlobalLeader);
          fprintf(stderr, "updateGlobalLeader: error couldn't set localStartTime for group: %u\n",
          gid);
          return -1;
        }
      }
    }
  }
  
  tmp_leader = malloc(sizeof(*tmp_leader));
  tmp_leader->pid = globalLeader->pid;
  tmp_leader->stable = globalLeader->stable;
  memcpy(&tmp_leader->addr, &globalLeader->addr, sizeof(struct sockaddr_in));
  
  if ((nb_remote_proc_in_contenders == 0) ||
  ((nb_remote_proc_in_contenders == 1) && (!is_local_address(&newGlobalLeader->addr))))
  globalLeader->stable = 1;
  else
    globalLeader->stable = 0;
  
  memcpy(&globalLeader->addr, &newGlobalLeader->addr, sizeof(struct sockaddr_in));
  globalLeader->pid = newGlobalLeader->pid;
  
#ifdef OMEGA_OUTPUT
  fprintf(stdout, "Old leader was pid: %u addr: %u.%u.%u.%u newleader is pid: %u addr: %u.%u.%u.%u\n",
    tmp_leader->pid, NIPQUAD(&tmp_leader->addr), globalLeader->pid,
  NIPQUAD(&globalLeader->addr));
#endif
#ifdef OMEGA_LOG
  fprintf(omega_log, "Old leader was pid: %u addr: %u.%u.%u.%u newleader is pid: %u addr: %u.%u.%u.%u\n",
    tmp_leader->pid, NIPQUAD(&tmp_leader->addr), globalLeader->pid,
  NIPQUAD(&globalLeader->addr));
#endif
  
  /* Notify processes interested in this group if the leader changed */
  stability_changed = (tmp_leader->stable != globalLeader->stable);
  addr_changed = !sockaddr_eq(&tmp_leader->addr, &globalLeader->addr);
  pid_changed = (tmp_leader->pid != globalLeader->pid);
  leader_changed = addr_changed || pid_changed || stability_changed;
  
  
  list_for_each(tmp_head1, &localregistered_proc_head) {
    rproc = list_entry(tmp_head1, struct localregistered_proc_struct, localproc_list);
    list_for_each(tmp_head2, &rproc->notif_type_list) {
      tmp_notif = list_entry(tmp_head2, struct notif_type_struct, notif_type_list);
      if (tmp_notif->gid == gid) {
        if (tmp_notif->notif_type == OMEGA_INTERRUPT_ANY_CHANGE) {
          if (leader_changed || !tmp_notif->already_notified) {
            msg_omega_build_notify(msg, &globalLeader->addr, globalLeader->pid, globalLeader->gid,
            globalLeader->stable);
            
            if (write_msg(rproc->omega_int_fd, msg, OMEGA_FIFO_MSG_LEN) < 0)
              fprintf(stderr, "omega updateGlobalLeader: error while writing to interrupt pipe\n");
            else
              tmp_notif->already_notified = 1;
          }
        }
        break;
      }
      else if (tmp_notif->gid > gid)
        break;
    }
  }
  
  
  free(tmp_leader);
  free(newGlobalLeader);
  return 0;
}

inline void doUponSuspected(struct sockaddr_in *addr, u_int pid, u_int gid, struct timeval *now) {
  
  struct timeval startTime;
  struct contenders_struct *tmp_contenders;
  struct proc_struct *tmp_proc;
  struct list_head *tmp_head1, *tmp_head2;
  int otherProcWithSameAddr = 0;
  
  /* If it's a local adress we don't need to remove the process from the contenders set
  and update the leader since it has already been done in omega_local_unreg. Furthermore,
   we don't need to send it an accusation msg. */
  
  if (!is_local_address(addr)) { /* remote address */
    remove_proc_from_globalContenders_set(addr, pid, gid);
    
    list_for_each(tmp_head1, &globalContenders_head) {
      tmp_contenders = list_entry(tmp_head1, struct contenders_struct, contenders_list);
      if (tmp_contenders->gid < gid)
        continue;
      else if (tmp_contenders->gid == gid) {
        list_for_each(tmp_head2, &tmp_contenders->procs_list) {
          tmp_proc = list_entry(tmp_head2, struct proc_struct, proc_list);
          if (sockaddr_eq(&tmp_proc->addr, addr)) {
            otherProcWithSameAddr = 1;
            break;
          }
        }
        break;
      }
      else {
        fprintf(stderr, "omega doUponSuspected: error while locating contenders set of group: %u\n", gid);
        break;
      }
    }
    
    
    if (!otherProcWithSameAddr) {
      if (get_startTime_of_remoteprocess(addr, gid, &startTime) < 0) {
        fprintf(stderr, "omega check fdd int: Error while getting remote startTime\n");
        fprintf(stderr, "of addr: %u.%u.%u.%u for group: %u\n", NIPQUAD(addr), gid);
      }
    }
    updateGlobalLeader(gid, now);
  }
}

inline void doUponReceivedAccusation(u_int gid, struct timeval *startTime, struct timeval *now) {
  
  struct timeval local_startTime;
  
  if (omega_group_exists_locally(gid, CANDIDATE)) {
    if (getlocalstartTime(gid, &local_startTime) == 0) {
      if (timercmp(startTime, &local_startTime, ==)) {
        if (set_local_accusationTime(gid) < 0)
        fprintf(stderr, "omega doUponReceivedAccusation: Error while setting local startTime of group: %u\n",
        gid);
        if (updateGlobalLeader(gid, now) < 0)
        fprintf(stderr, "omega doUponReceivedAccusation: Error while updating leader of group gid: %u\n",
        gid);
      }
    }
    else
      fprintf(stderr, "omega doUponReceivedAccusation: Error while getting local startTime\n");
  }
}

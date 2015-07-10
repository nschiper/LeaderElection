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

/* The list of list of actives (one list per group) */
struct list_head actives_head;


int omega_algorithm_init() {
  
  INIT_LIST_HEAD(&(localLeader_head));
  INIT_LIST_HEAD(&(localContenders_head));
  INIT_LIST_HEAD(&(globalLeader_head));
  INIT_LIST_HEAD(&(actives_head));
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



inline int add_or_replace_locaLeader_in_actives_set(u_int pid, u_int gid) {
  
  struct contenders_struct *entry_ptr = NULL;
  int entry_exists;
  struct proc_struct *tmp_proc;
  struct list_head *tmp_head;
  int found_proc = 0;
  
  list_insert_ordered(gid, gid, &actives_head, struct contenders_struct, contenders_list, <);
  
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


inline int add_proc_in_actives_set(struct sockaddr_in *addr, u_int pid, u_int gid) {
  
  struct contenders_struct *entry_ptr = NULL;
  int entry_exists;
  struct proc_struct *tmp_proc;
  struct list_head *tmp_head;
  int found_proc = 0;
  
  list_insert_ordered(gid, gid, &actives_head, struct contenders_struct, contenders_list, <);
  
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

inline int remove_proc_from_actives_set(struct sockaddr_in *addr, u_int pid, u_int gid) {
  
  struct list_head *tmp_head1, *tmp_head2;
  struct contenders_struct *tmp_actives;
  struct proc_struct *tmp_proc;
  
  list_for_each(tmp_head1, &actives_head) {
    tmp_actives = list_entry(tmp_head1, struct contenders_struct, contenders_list);
    if (tmp_actives->gid < gid)
      continue;
    else if (tmp_actives->gid == gid) {
      list_for_each(tmp_head2, &tmp_actives->procs_list) {
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


/* Returns 1 if the proc2 is "smaller" than proc1, 0 otherwise.
To determine which one is the smallest, we proceed in the following way:
1) compare their accusationTime variable, if they are equal goto 2)
 2) compare their IP address. */
static inline int compare_procs(struct bestAmongActives_struct *proc1, struct timeval *proc1_accusationTime,
  struct proc_struct *proc2, struct timeval *proc2_accusationTime) {
  if (timercmp(proc2_accusationTime, proc1_accusationTime, <))
    return 1;
  else if (timercmp(proc2_accusationTime, proc1_accusationTime, ==)) {
    if (sockaddr_smaller(&proc2->addr, &proc1->addr))
      return 1;
    else
      return 0;
  }
  else
    return 0;
}


/* Returns 1 if the bestAmongActives_proc is "smaller" than the current temp_newleader, 0 otherwise.
To determine which one is the smallest, we proceed in the following way:
1) compare their accusationTime variable, if they are equal goto 2)
 2) compare their IP address. */
static inline int compare_bestAmongActives(struct leaders_struct *temp_newleader, struct timeval *newleader_accusationTime,
  struct bestAmongActives_struct *bestAmongActives_proc,
  struct timeval *bestAmongActives_proc_accusationTime) {
  if (timercmp(bestAmongActives_proc_accusationTime, newleader_accusationTime, <))
    return 1;
  else if (timercmp(bestAmongActives_proc_accusationTime,
    newleader_accusationTime, ==)) {
    if (sockaddr_smaller(&bestAmongActives_proc->addr, &temp_newleader->addr))
      return 1;
    else
      return 0;
  }
  else
    return 0;
}


inline int updateLocalLeader(unsigned int gid) {
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
    
    /* The newLocalLeader is the first process of the local contenders set. */
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
    if (add_or_replace_locaLeader_in_actives_set(newLocalLeader->pid, gid) < 0)
      fprintf(stderr, "updateLocalLeader: error while adding or replacing localLeader in actives\n");
    
    if (updateGlobalLeader(gid) < 0) {
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
          msg_omega_build_notify(msg, &globalLeader->addr, globalLeader->pid, globalLeader->gid);
          
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

int updateGlobalLeaders() {
  
  struct contenders_struct *tmp_actives;
  struct list_head *tmp_head;
  
  list_for_each(tmp_head, &actives_head) {
    tmp_actives = list_entry(tmp_head, struct contenders_struct, contenders_list);
    if (updateGlobalLeader(tmp_actives->gid) < 0)
      return -1;
  }
  return 0;
}

inline int updateGlobalLeader(unsigned int gid) {
  
  struct leaders_struct *globalLeader = NULL, *newGlobalLeader, *tmp_leader;
  struct contenders_struct *tmp_actives;
  struct proc_struct *tmp_proc;
  struct bestAmongActives_struct *bestAmongActives, *localBestAmongActives;
  struct bestAmongActives_struct tmpBestAmongActives;
  struct list_head *tmp_head1, *tmp_head2, *tmp_head3;
  struct timeval accusationTime1, accusationTime2;
  int globalLeader_found = 0;
  struct localregistered_proc_struct *rproc;
  struct notif_type_struct *tmp_notif;
  char msg[OMEGA_FIFO_MSG_LEN];
  int addr_changed, pid_changed, leader_changed;
  
  
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
    
    if (globalLeader == NULL)
      return -1;
    else {
      globalLeader->gid = gid;
      globalLeader->pid = 0; /* leader address and pid not yet assigned */
      memset(&globalLeader->addr, 0, sizeof(struct sockaddr_in));
      list_add_tail(&(globalLeader->leaders_list), tmp_head1);
    }
  }
  
  bestAmongActives = malloc(sizeof(*bestAmongActives));
  bestAmongActives->pid = 0; /* bestAmongActives address and pid not yet assigned */
  
  /*********************************** Get the bestAmongActives ***********************************************/
  list_for_each(tmp_head2, &actives_head) {
    tmp_actives = list_entry(tmp_head2, struct contenders_struct, contenders_list);
    if (tmp_actives->gid < gid)
      continue;
    else if (tmp_actives->gid == gid) {
      list_for_each(tmp_head3, &tmp_actives->procs_list) {
        tmp_proc = list_entry(tmp_head3, struct proc_struct, proc_list);
        /* The process is local */
        if (is_local_address(&tmp_proc->addr)) {
          if (getlocalaccusationTime(gid, &accusationTime1) < 0) {
            free(bestAmongActives);
            fprintf(stderr, "updateGlobalLeader: error couldn't find local accusationTime for group: %u\n", gid);
            return -1;
          }
          else {
            if (bestAmongActives->pid == 0) { /* bestAmongActives not yet initialized */
              bestAmongActives->pid = tmp_proc->pid;
              memcpy(&bestAmongActives->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
            }
            else if (is_local_address(&bestAmongActives->addr)) { /* temporary bestAmongActives is local */
              if (getlocalaccusationTime(gid, &accusationTime2) < 0) {
                fprintf(stderr, "updateGlobalLeader: error couldn't find local accusationTime for gid: %u\n", gid);
                free(bestAmongActives);
                return -1;
              }
              else {
                if(compare_procs(bestAmongActives, &accusationTime2, tmp_proc, &accusationTime1)) {
                  bestAmongActives->pid = tmp_proc->pid;
                  memcpy(&bestAmongActives->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
                }
              }
            }
            else { /* temporary bestAmongActives is remote */
              if (get_accusationTime_of_remoteprocess(&bestAmongActives->addr, gid, &accusationTime2) < 0) {
                fprintf(stderr, "updateGlobalLeader: error couldn't find remote accusationTime for group: %u and addr: %u.%u.%u.%u\n",
                gid, NIPQUAD(&bestAmongActives->addr));
                printf("get accTime of bestAmongActives 1\n");
                free(bestAmongActives);
                return -1;
              }
              else {
                if(compare_procs(bestAmongActives, &accusationTime2, tmp_proc, &accusationTime1)) {
                  bestAmongActives->pid = tmp_proc->pid;
                  memcpy(&bestAmongActives->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
                }
              }
            }
          }
        }
        /* The process is remote */
        else {
          if (get_accusationTime_of_remoteprocess(&tmp_proc->addr, gid, &accusationTime1) < 0) {
            fprintf(stderr, "updateGlobalLeader: error couldn't find remote accusationTime for group: %u and addr: %u.%u.%u.%u\n",
            gid, NIPQUAD(&tmp_proc->addr));
            printf("get accTime of remote process\n");
            free(bestAmongActives);
            return -1;
          }
          else {
            if (bestAmongActives->pid == 0) { /* bestAmongActives not yet initialized */
              bestAmongActives->pid = tmp_proc->pid;
              memcpy(&bestAmongActives->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
            }
            else if (is_local_address(&bestAmongActives->addr)) { /* temporary bestAmongActives is local */
              if (getlocalaccusationTime(gid, &accusationTime2) < 0) {
                fprintf(stderr, "updateGlobalLeader: error couldn't find local counter of fo group: %u\n", gid);
                free(bestAmongActives);
                return -1;
              }
              else {
                if(compare_procs(bestAmongActives, &accusationTime2, tmp_proc, &accusationTime1)) {
                  bestAmongActives->pid = tmp_proc->pid;
                  memcpy(&bestAmongActives->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
                }
              }
            }
            else { /* temporary bestAmongActives is remote */
              if (get_accusationTime_of_remoteprocess(&bestAmongActives->addr, gid, &accusationTime2) < 0) {
                fprintf(stderr, "updateGlobalLeader: error couldn't find remote accusationTime for group: %u and addr: %u.%u.%u.%u\n",
                gid, NIPQUAD(&bestAmongActives->addr));
                printf("get accTime of bestAmongActives 2\n");
                free(bestAmongActives);
                return -1;
              }
              else {
                if(compare_procs(bestAmongActives, &accusationTime2, tmp_proc, &accusationTime1)) {
                  bestAmongActives->pid = tmp_proc->pid;
                  memcpy(&bestAmongActives->addr, &tmp_proc->addr, sizeof(struct sockaddr_in));
                }
              }
            }
          }
        }
      }
      break;
    }
    else
      fprintf(stderr, "omega updateGlobalLeader: error couldn't find actives set for group: %u\n", gid);
  }
  
  localBestAmongActives = malloc(sizeof(*localBestAmongActives));
  if (getlocalbestAmongActives(gid, localBestAmongActives) == 0) {
    
    /* Send a report message to every process in the group whenever
    localBestAmongActives changes.
     This should reduce Tglobal detection. */
    if ((localBestAmongActives->pid != bestAmongActives->pid) ||
      (!sockaddr_eq(&localBestAmongActives->addr, &bestAmongActives->addr))) {
      
      struct timeval now;
      timerclear(&now);
      sched_report_group_now(gid, &now);
    }
  }
  
  
  set_local_bestAmongActives(gid, bestAmongActives);
  
  
#ifdef OMEGA_OUTPUT
  fprintf(stdout, "omega updateGlobalLeader of group: %u, new bestAmongActives pid: %u addr: %u.%u.%u.%u\n",
  gid, bestAmongActives->pid, NIPQUAD(&bestAmongActives->addr));
#endif
#ifdef OMEGA_LOG
  fprintf(omega_log, "omega updateGlobalLeader of group: %u, new bestAmongActives pid: %u addr: %u.%u.%u.%u\n",
  gid, bestAmongActives->pid, NIPQUAD(&bestAmongActives->addr));
#endif
  
  /**************************************************************************************************************/
  
  
  /*********************************** Get the new Global leader ***********************************************/
  
  newGlobalLeader = malloc(sizeof(*newGlobalLeader));
  newGlobalLeader->gid = gid;
  newGlobalLeader->pid = 0; /* leader address and pid not yet assigned */
  memset(&newGlobalLeader->addr, 0x0, sizeof(struct sockaddr_in));
  
  
  list_for_each(tmp_head2, &actives_head) {
    tmp_actives = list_entry(tmp_head2, struct contenders_struct, contenders_list);
    if (tmp_actives->gid < gid)
      continue;
    else if (tmp_actives->gid == gid) {
      list_for_each(tmp_head3, &tmp_actives->procs_list) {
        tmp_proc = list_entry(tmp_head3, struct proc_struct, proc_list);
        
        /* The proc is local */
        if (is_local_address(&tmp_proc->addr)) {
          if (getlocalbestAmongActives(gid, &tmpBestAmongActives) < 0) {
            free(bestAmongActives);
            free(newGlobalLeader);
            fprintf(stderr, "updateGlobalLeader: error couldn't find local bestAmongActives of group: %u\n", gid);
            return -1;
          }
          if (is_local_address(&(tmpBestAmongActives.addr))) {
            if (getlocalaccusationTime(gid, &accusationTime1) < 0) {
              free(bestAmongActives);
              free(newGlobalLeader);
              fprintf(stderr, "updateGlobalLeader: error couldn't find local accusationTime for group: %u\n", gid);
              return -1;
            }
          }
          else {
            if (get_accusationTime_of_remoteprocess(&(tmpBestAmongActives.addr), gid, &accusationTime1) < 0) {
              fprintf(stderr, "updateGlobalLeader: error couldn't find remote accusationTime for group: %u and addr: %u.%u.%u.%u\n",
              gid, NIPQUAD(&(tmpBestAmongActives.addr)));
              printf("get accTime of tmpBestAmongActives 1\n");
              free(bestAmongActives);
              free(newGlobalLeader);
              return -1;
            }
          }
          
          if (newGlobalLeader->pid == 0) { /* newGlobalLeader not yet initialized */
            newGlobalLeader->pid = tmpBestAmongActives.pid;
            memcpy(&newGlobalLeader->addr, &(tmpBestAmongActives.addr), sizeof(struct sockaddr_in));
          }
          else if (is_local_address(&newGlobalLeader->addr)) { /* temporary newGlobalLeader is local */
            if (getlocalaccusationTime(gid, &accusationTime2) < 0) {
              fprintf(stderr, "updateGlobalLeader: error couldn't find local accusationTime for gid: %u\n", gid);
              free(bestAmongActives);
              free(newGlobalLeader);
              return -1;
            }
            else {
              if(compare_bestAmongActives(newGlobalLeader, &accusationTime2,
                &tmpBestAmongActives, &accusationTime1)) {
                newGlobalLeader->pid = tmpBestAmongActives.pid;
                memcpy(&newGlobalLeader->addr, &(tmpBestAmongActives.addr),
                sizeof(struct sockaddr_in));
              }
            }
          }
          else { /* temporary newGlobalLeader is remote */
            if (get_accusationTime_of_remoteprocess(&newGlobalLeader->addr, gid, &accusationTime2) < 0) {
              fprintf(stderr, "updateGlobalLeader: error couldn't find remote accusationTime for group: %u and addr: %u.%u.%u.%u\n",
              gid, NIPQUAD(&newGlobalLeader->addr));
              printf("get accTime of newGlobalLeader 1\n");
              free(bestAmongActives);
              free(newGlobalLeader);
              return -1;
            }
            else {
              if(compare_bestAmongActives(newGlobalLeader, &accusationTime2,
                &tmpBestAmongActives, &accusationTime1)) {
                newGlobalLeader->pid = tmpBestAmongActives.pid;
                memcpy(&newGlobalLeader->addr, &(tmpBestAmongActives.addr),
                sizeof(struct sockaddr_in));
              }
            }
          }
        }
        /* The proc is remote */
        else {
          if (get_bestAmongActives_of_remoteprocess(&tmp_proc->addr, gid, &tmpBestAmongActives) < 0) {
            free(bestAmongActives);
            free(newGlobalLeader);
            fprintf(stderr, "updateGlobalLeader: error couldn't find remote bestAmongActives of machine: %u.%u.%u.%u for group: %u\n",
            NIPQUAD(&tmp_proc->addr), gid);
            return -1;
          }
          if (is_local_address(&(tmpBestAmongActives.addr))) {
            if (getlocalaccusationTime(gid, &accusationTime1) < 0) {
              free(bestAmongActives);
              free(newGlobalLeader);
              fprintf(stderr, "updateGlobalLeader: error couldn't find local accusationTime for group: %u\n", gid);
              return -1;
            }
          }
          else {
            if (get_accusationTime_of_remoteprocess(&(tmpBestAmongActives.addr), gid, &accusationTime1) < 0) {
              fprintf(stderr, "updateGlobalLeader: error couldn't find remote accusationTime for group: %u and addr: %u.%u.%u.%u\n",
              gid, NIPQUAD(&(tmpBestAmongActives.addr)));
              printf("get accTime of tmpBestAmongActives 2\n");
              free(bestAmongActives);
              free(newGlobalLeader);
              return -1;
            }
          }
          
          if (newGlobalLeader->pid == 0) { /* newGlobalLeader not yet initialized */
            newGlobalLeader->pid = tmpBestAmongActives.pid;
            memcpy(&newGlobalLeader->addr, &(tmpBestAmongActives.addr), sizeof(struct sockaddr_in));
          }
          else if (is_local_address(&newGlobalLeader->addr)) { /* temporary newGlobalLeader is local */
            if (getlocalaccusationTime(gid, &accusationTime2) < 0) {
              fprintf(stderr, "updateGlobalLeader: error couldn't find local counter of fo group: %u\n", gid);
              free(bestAmongActives);
              free(newGlobalLeader);
              return -1;
            }
            else {
              if(compare_bestAmongActives(newGlobalLeader, &accusationTime2,
                &tmpBestAmongActives, &accusationTime1)) {
                newGlobalLeader->pid = tmpBestAmongActives.pid;
                memcpy(&newGlobalLeader->addr, &(tmpBestAmongActives.addr), sizeof(struct sockaddr_in));
              }
            }
          }
          else { /* temporary newGlobalLeader is remote */
            if (get_accusationTime_of_remoteprocess(&newGlobalLeader->addr, gid, &accusationTime2) < 0) {
              fprintf(stderr, "updateGlobalLeader: error couldn't find remote accusationTime for group: %u and addr: %u.%u.%u.%u\n",
              gid, NIPQUAD(&newGlobalLeader->addr));
              printf("get accTime of newGlobalLeader 2\n");
              free(bestAmongActives);
              free(newGlobalLeader);
              return -1;
            }
            else {
              if(compare_bestAmongActives(newGlobalLeader, &accusationTime2,
                &tmpBestAmongActives, &accusationTime1)) {
                newGlobalLeader->pid = tmpBestAmongActives.pid;
                memcpy(&newGlobalLeader->addr, &(tmpBestAmongActives.addr), sizeof(struct sockaddr_in));
              }
            }
          }
        }
      }
      break;
    }
    else
      fprintf(stderr, "omega updateGlobalLeader: error couldn't find actives set for group: %u\n", gid);
  }
  
#ifdef OMEGA_OUTPUT
  fprintf(stdout, "omega updateGlobalLeader of group: %u, new globalLeader pid: %u addr: %u.%u.%u.%u\n",
  gid, newGlobalLeader->pid, NIPQUAD(&newGlobalLeader->addr));
#endif
#ifdef OMEGA_LOG
  fprintf(omega_log, "omega updateGlobalLeader of group: %u, new globalLeader pid: %u addr: %u.%u.%u.%u\n",
  gid, newGlobalLeader->pid, NIPQUAD(&newGlobalLeader->addr));
#endif
  
  /**************************************************************************************************************/
  
  tmp_leader = malloc(sizeof(*tmp_leader));
  tmp_leader->pid = globalLeader->pid;
  memcpy(&tmp_leader->addr, &globalLeader->addr, sizeof(struct sockaddr_in));
  
  
  memcpy(&globalLeader->addr, &newGlobalLeader->addr, sizeof(struct sockaddr_in));
  globalLeader->pid = newGlobalLeader->pid;
  
  
  /* Notify processes interested in this group if the leader changed */
  addr_changed = !sockaddr_eq(&tmp_leader->addr, &globalLeader->addr);
  pid_changed = (tmp_leader->pid != globalLeader->pid);
  leader_changed = (addr_changed || pid_changed);
  
  
  list_for_each(tmp_head1, &localregistered_proc_head) {
    rproc = list_entry(tmp_head1, struct localregistered_proc_struct, localproc_list);
    list_for_each(tmp_head2, &rproc->notif_type_list) {
      tmp_notif = list_entry(tmp_head2, struct notif_type_struct, notif_type_list);
      if (tmp_notif->gid == gid) {
        if (tmp_notif->notif_type == OMEGA_INTERRUPT_ANY_CHANGE) {
          if (leader_changed || !tmp_notif->already_notified) {
            msg_omega_build_notify(msg, &globalLeader->addr, globalLeader->pid, globalLeader->gid);
            
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
  free(bestAmongActives);
  free(localBestAmongActives);
  return 0;
}

inline void doUponSuspected(struct sockaddr_in *addr, u_int pid, u_int gid, struct timeval *now) {
  
  struct contenders_struct *tmp_actives;
  struct proc_struct *tmp_proc;
  struct list_head *tmp_head1, *tmp_head2;
  int otherProcWithSameAddr = 0;
  
  /* If it's a local adress we don't need to remove the process from the actives set
  and update the leader since it has already been done in omega_local_unreg. Furthermore,
   we don't need to send it an accusation msg. */
  if (!is_local_address(addr)) { /* remote address */
    remove_proc_from_actives_set(addr, pid, gid);
    
    list_for_each(tmp_head1, &actives_head) {
      tmp_actives = list_entry(tmp_head1, struct contenders_struct, contenders_list);
      if (tmp_actives->gid < gid)
        continue;
      else if (tmp_actives->gid == gid) {
        list_for_each(tmp_head2, &tmp_actives->procs_list) {
          tmp_proc = list_entry(tmp_head2, struct proc_struct, proc_list);
          if (sockaddr_eq(&tmp_proc->addr, addr)) {
            otherProcWithSameAddr = 1;
            break;
          }
        }
        break;
      }
      else {
        fprintf(stderr, "omega doUponSuspected: error while locating actives set of group: %u\n", gid);
        break;
      }
    }
    
    
    if (!otherProcWithSameAddr) {
      send_accusation(addr, gid);
    }
    
    updateGlobalLeader(gid);
  }
}

inline void doUponReceivedAccusation(u_int gid) {
  
  /* We first check if the group gid exists locally. */
  if (omega_group_exists_locally(gid, CANDIDATE)) {
    if (set_local_accusationTime(gid) < 0)
    fprintf(stderr, "omega doUponReceivedAccusation: Error while setting local startTime of group: %u\n",
    gid);
    if (updateGlobalLeader(gid) < 0)
    fprintf(stderr, "omega doUponReceivedAccusation: Error while updating leader of group gid: %u\n",
    gid);
  }
}

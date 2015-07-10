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


#include <string.h>
#include "variables_exchange.h"
#include "misc.h"
#include "fdd_types.h"
#include <sys/time.h>
#include <netinet/in.h>


int exchange_vars_init() {
  /* initialize the localvars list and remotevars list */
  INIT_LIST_HEAD(&(localvars_head));
  INIT_LIST_HEAD(&(remotevars_head));
  return 0;
}



/*****************************************************************************/
/* The following procedures are utilities procedures that deal with          */
/* remotevars lists (not the global variable!)                               */
/*****************************************************************************/

/* Inserts remote variables in a remotevars list. */
inline int insert_remotevars_in_list(struct sockaddr_in *addr, u_int gid, struct timeval *accusationTime,
  struct bestAmongActives_struct *bestAmongActives,
  struct timeval *bestAmongActives_accTime,
  struct list_head *remotevars) {
  
  struct remotevars_struct *tmp_remotevars = NULL;
  struct list_head *tmp_head1 = NULL;
  struct vars_struct *tmp_vars = NULL;
  struct list_head *tmp_head2 = NULL;
  int found_addr = 0, found_gid = 0;
  
  list_for_each(tmp_head1, remotevars) {
    tmp_remotevars = list_entry(tmp_head1, struct remotevars_struct, remotevars_list);
    if (sockaddr_smaller(&tmp_remotevars->addr, addr))
      continue;
    else if (sockaddr_eq(&tmp_remotevars->addr, addr)) {
      found_addr = 1;
      list_for_each(tmp_head2, &tmp_remotevars->vars_head) {
        tmp_vars = list_entry(tmp_head2, struct vars_struct, vars_list);
        if (tmp_vars->gid < gid)
          continue;
        else if (tmp_vars->gid == gid) {
          found_gid = 1;
          
          /* the max is taken because the links are not
           necessarily fifo */
          memcpy(&tmp_vars->accusationTime, timermax(accusationTime,
          &tmp_vars->accusationTime), sizeof(struct timeval));
          
          if (bestAmongActives_accTime != NULL) {
            
            /* If bestAmongActive of machine addr is itself there is no point in inserting
             bestAmongActive_accTime */
            if (!sockaddr_eq(addr, &bestAmongActives->addr)) {
              if (insert_remotevars_in_list(&bestAmongActives->addr, gid, bestAmongActives_accTime,
              NULL, NULL, remotevars) < 0)
              fprintf(stderr, "Error in insert_remotevars_in_list()\n");
            }
            
            tmp_vars->bestAmongActives.pid = bestAmongActives->pid;
            memcpy(&(tmp_vars->bestAmongActives.addr), &bestAmongActives->addr,
            sizeof(struct sockaddr_in));
          }
          break;
        }
        else
          break;
      }
      if (!found_gid) {
        /* New gid for that remote host */
        tmp_vars = malloc(sizeof(struct vars_struct));
        if (tmp_vars == NULL)
          return -1;
        tmp_vars->gid = gid;
        memcpy(&tmp_vars->accusationTime, accusationTime, sizeof(struct timeval));
        
        if (bestAmongActives_accTime != NULL) {
          tmp_vars->bestAmongActives.pid = bestAmongActives->pid;
          memcpy(&(tmp_vars->bestAmongActives.addr), &bestAmongActives->addr,
          sizeof(struct sockaddr_in));
        }
        else {
          /* set everything to zero */
          tmp_vars->bestAmongActives.pid = 0;
          memset(&(tmp_vars->bestAmongActives.addr), 0, sizeof(struct sockaddr_in));
        }
        
        list_add_tail(&(tmp_vars->vars_list), tmp_head2);
        
        /* If bestAmongActive of machine addr is itself there is no point in inserting
         bestAmongActive_accTime */
        if (bestAmongActives_accTime != NULL) {
          if (!sockaddr_eq(addr, &bestAmongActives->addr)) {
            if (insert_remotevars_in_list(&bestAmongActives->addr, gid, bestAmongActives_accTime,
            NULL, NULL, remotevars) < 0)
            fprintf(stderr, "Error in insert_remotevars_in_list()\n");
          }
        }
      }
      break;
    }
    else
      break;
  }
  
  if (!found_addr) {
    tmp_remotevars = malloc(sizeof(struct remotevars_struct));
    if (tmp_remotevars == NULL)
      return -1;
    memcpy(&tmp_remotevars->addr, addr, sizeof(struct sockaddr_in));
    INIT_LIST_HEAD(&tmp_remotevars->vars_head);
    tmp_vars = malloc(sizeof(struct vars_struct));
    if (tmp_vars == NULL) {
      free(tmp_remotevars);
      return -1;
    }
    tmp_vars->gid = gid;
    memcpy(&tmp_vars->accusationTime, accusationTime, sizeof(struct timeval));
    
    if (bestAmongActives_accTime != NULL) {
      tmp_vars->bestAmongActives.pid = bestAmongActives->pid;
      memcpy(&(tmp_vars->bestAmongActives.addr), &bestAmongActives->addr,
      sizeof(struct sockaddr_in));
    }
    else {
      /* set everything to zero */
      tmp_vars->bestAmongActives.pid = 0;
      memset(&(tmp_vars->bestAmongActives.addr), 0, sizeof(struct sockaddr_in));
    }
    
    list_add_tail(&(tmp_vars->vars_list), &tmp_remotevars->vars_head);
    list_add_tail(&(tmp_remotevars->remotevars_list), tmp_head1);
    
    /* If bestAmongActive of machine addr is itself there is no point in inserting
     bestAmongActive_accTime */
    if (bestAmongActives_accTime != NULL) {
      if (!sockaddr_eq(addr, &bestAmongActives->addr)) {
        if (insert_remotevars_in_list(&bestAmongActives->addr, gid, bestAmongActives_accTime,
        NULL, NULL, remotevars) < 0)
        fprintf(stderr, "Error in insert_remotevars_in_list()\n");
      }
    }
  }
  return 0;
}



void print_remotvars_list(FILE *stream, struct list_head *remotevars_list) {
  struct remotevars_struct *tmp_remote;
  struct list_head *tmp_head1;
  struct vars_struct *tmp_vars;
  struct list_head *tmp_head2;
  
  fprintf(stream, "\n ** Printing remotevars list: **\n");
  list_for_each(tmp_head1, remotevars_list) {
    tmp_remote = list_entry(tmp_head1, struct remotevars_struct, remotevars_list);
    fprintf(stream, "vars for host: %u.%u.%u.%u\n", NIPQUAD(&tmp_remote->addr));
    list_for_each(tmp_head2, &tmp_remote->vars_head) {
      tmp_vars = list_entry(tmp_head2, struct vars_struct, vars_list);
      fprintf(stream, "\tgid: %u accusationTime: %ld.%ld\n", tmp_vars->gid,
      (tmp_vars->accusationTime).tv_sec, (tmp_vars->accusationTime).tv_usec);
      fprintf(stream, "\tbestAmongActives pid: %u addr: %u.%u.%u.%u\n", tmp_vars->bestAmongActives.pid,
      NIPQUAD(&(tmp_vars->bestAmongActives.addr)));
    }
  }
  fprintf(stream, " ** End of remotevars list printing **\n");
}


void free_remotevars_list(struct list_head *remotevars_list) {
  
  struct list_head *tmp_head1;
  struct remotevars_struct *remote_tmp;
  
  list_for_each(tmp_head1, remotevars_list) {
    remote_tmp = list_entry(tmp_head1, struct remotevars_struct, remotevars_list);
    tmp_head1 = tmp_head1->prev;
    list_del(&remote_tmp->remotevars_list);
    list_free(&remote_tmp->vars_head, struct vars_struct, vars_list);
    free(remote_tmp);
  }
}


/*****************************************************************************/
/* End of utilities procedures                                               */
/*****************************************************************************/



/*****************************************************************************/
/* The following procedures procedures deal with the localvars list          */
/*****************************************************************************/

/* Creates the localvars of group gid */
inline int create_localvars(unsigned int gid) {
  struct vars_struct *entry_ptr = NULL;
  int entry_exists;
  struct timeval tv;
  
  list_insert_ordered(gid, gid, &localvars_head, struct vars_struct, vars_list, <);
  
  if (entry_ptr == NULL) {
    return -1;
  }
  else {
    if (entry_exists)
      fprintf(stderr, "create_localvars: error group: %u already exists\n", gid);
    else {
      if (gettimeofday(&tv, NULL) < 0)
        fprintf(stderr, "create_localvars: error while getting current time\n");
      else {
        memcpy(&entry_ptr->accusationTime, &tv, sizeof(struct timeval));
        entry_ptr->bestAmongActives.pid = 0;
        memset(&(entry_ptr->bestAmongActives.addr), 0, sizeof(struct sockaddr_in));
      }
    }
  }
  return 0;
}

/* Removes the localvars for group gid. */
inline void remove_localvars(unsigned int gid) {
  int found;
  list_remove_ordered(gid, gid, &localvars_head, struct vars_struct, vars_list, <);
}


/* checks if the localvars for group gid exist. */
inline int localvars_exist(unsigned int gid) {
  
  struct vars_struct *tmp_vars = NULL;
  struct list_head *tmp_head = NULL;
  
  list_for_each(tmp_head, &localvars_head) {
    tmp_vars = list_entry(tmp_head, struct vars_struct, vars_list);
    if (tmp_vars->gid < gid)
      continue;
    else if (tmp_vars->gid == gid) {
      return 1;
    }
    else if (tmp_vars->gid > gid)
      return 0;
  }
  return 0;
}

/* Inserts the localvars of group gid in accusationTime and bestAmongActives. If no
 such group exists it returns -1 and 0 otherwise. */
inline int getlocalvars(unsigned int gid, struct timeval *accusationTime,
  struct bestAmongActives_struct *bestAmongActives) {
  int found = 0;
  struct vars_struct *tmp_vars = NULL;
  struct list_head *tmp_head = NULL;
  
  list_for_each(tmp_head, &localvars_head) {
    tmp_vars = list_entry(tmp_head, struct vars_struct, vars_list);
    if (tmp_vars->gid < gid)
      continue;
    else if (tmp_vars->gid == gid) {
      found = 1;
      memcpy(accusationTime, &tmp_vars->accusationTime, sizeof(struct timeval));
      memcpy(bestAmongActives, &tmp_vars->bestAmongActives, sizeof(struct bestAmongActives_struct));
      break;
    }
    else if (tmp_vars->gid > gid)
      break;
  }
  
  if (found)
    return 0;
  else
    return -1;
}

inline int getlocalaccusationTime(unsigned int gid, struct timeval *accusationTime) {
  
  int found = 0;
  struct vars_struct *tmp_vars = NULL;
  struct list_head *tmp_head = NULL;
  
  list_for_each(tmp_head, &localvars_head) {
    tmp_vars = list_entry(tmp_head, struct vars_struct, vars_list);
    if (tmp_vars->gid < gid)
      continue;
    else if (tmp_vars->gid == gid) {
      found = 1;
      memcpy(accusationTime, &tmp_vars->accusationTime, sizeof(struct timeval));
      break;
    }
    else if (tmp_vars->gid > gid)
      break;
  }
  
  if (found)
    return 0;
  else
    return -1;
}

inline int getlocalbestAmongActives(unsigned int gid, struct bestAmongActives_struct *bestAmongActives) {
  
  int found = 0;
  struct vars_struct *tmp_vars = NULL;
  struct list_head *tmp_head = NULL;
  
  list_for_each(tmp_head, &localvars_head) {
    tmp_vars = list_entry(tmp_head, struct vars_struct, vars_list);
    if (tmp_vars->gid < gid)
      continue;
    else if (tmp_vars->gid == gid) {
      found = 1;
      memcpy(bestAmongActives, &tmp_vars->bestAmongActives, sizeof(struct bestAmongActives_struct));
      break;
    }
    else if (tmp_vars->gid > gid)
      break;
  }
  
  if (found)
    return 0;
  else
    return -1;
}


/* Adds delta_t microseconds to tv */
inline void inc_timer(struct timeval *tv, u_int delta_t) {
  tv->tv_usec += delta_t;
  if (tv->tv_usec >= 1000000) {
    tv->tv_sec++;
    tv->tv_usec -= 1000000;
  }
}

/* Sets the accusationTime of local group gid to max(accusationTime, localTime) + 1. The procedure returns -1 if
 no such process exist, 0 otherwise. */
inline int set_local_accusationTime(unsigned int gid) {
  
  int found = 0;
  struct vars_struct *tmp_vars = NULL;
  struct list_head *tmp_head = NULL;
  struct timeval now, *max;
  
  list_for_each(tmp_head, &localvars_head) {
    tmp_vars = list_entry(tmp_head, struct vars_struct, vars_list);
    if (tmp_vars->gid < gid)
      continue;
    else if (tmp_vars->gid == gid) {
      found = 1;
      if (gettimeofday(&now, NULL) < 0)
        fprintf(stderr, "set_local_accusationTime: error while getting current time\n");
      else {
        max = timermax(&now, &tmp_vars->accusationTime);  /* local clock may be non-monotonically increasing. */
        inc_timer(max, 1);
        memcpy(&tmp_vars->accusationTime, max, sizeof(struct timeval));
        break;
      }
    }
    else if (tmp_vars->gid > gid)
      break;
  }
  
  if (found)
    return 0;
  else
    return -1;
}


inline int set_local_bestAmongActives(unsigned int gid, struct bestAmongActives_struct *bestAmongActives) {
  
  int found = 0;
  struct vars_struct *tmp_vars = NULL;
  struct list_head *tmp_head = NULL;
  
  list_for_each(tmp_head, &localvars_head) {
    tmp_vars = list_entry(tmp_head, struct vars_struct, vars_list);
    if (tmp_vars->gid < gid)
      continue;
    else if (tmp_vars->gid == gid) {
      found = 1;
      tmp_vars->bestAmongActives.pid = bestAmongActives->pid;
      memcpy(&(tmp_vars->bestAmongActives.addr), &bestAmongActives->addr,
      sizeof(struct sockaddr_in));
    }
    else if (tmp_vars->gid > gid)
      break;
  }
  
  if (found)
    return 0;
  else
    return -1;
}


/*****************************************************************************/
/* End of procedures dealing with the localvars list                         */
/*****************************************************************************/

/*****************************************************************************/
/* The following procedures deal with the remotevars list                    */
/*****************************************************************************/

/* Returns 0 if the accusation variable of the group gid has been found,
 -1 otherwise. */
inline int get_accusationTime_of_remoteprocess(struct sockaddr_in *addr, u_int gid,
  struct timeval *accusationTime) {
  
  struct list_head *tmp_head1, *tmp_head2;
  
  struct remotevars_struct *tmp_remote;
  struct vars_struct *tmp_vars;
  
  list_for_each(tmp_head1, &remotevars_head) {
    tmp_remote = list_entry(tmp_head1, struct remotevars_struct, remotevars_list);
    if (sockaddr_smaller(&tmp_remote->addr, addr))
      continue;
    else if (sockaddr_eq(&tmp_remote->addr, addr)) {
      list_for_each(tmp_head2, &tmp_remote->vars_head) {
        tmp_vars = list_entry(tmp_head2, struct vars_struct, vars_list);
        if (tmp_vars->gid < gid)
          continue;
        else if (tmp_vars->gid == gid) {
          memcpy(accusationTime, &tmp_vars->accusationTime, sizeof(struct timeval));
          return 0;
        }
        else
          break;
      }
      break;
    }
    else
      break;
  }
  return -1;
}


/* Returns 0 if the bestAmongActives variable of the group gid has been found,
 -1 otherwise. */
inline int get_bestAmongActives_of_remoteprocess(struct sockaddr_in *addr, u_int gid,
  struct bestAmongActives_struct *bestAmongActives) {
  
  struct list_head *tmp_head1, *tmp_head2;
  
  struct remotevars_struct *tmp_remote;
  struct vars_struct *tmp_vars;
  
  list_for_each(tmp_head1, &remotevars_head) {
    tmp_remote = list_entry(tmp_head1, struct remotevars_struct, remotevars_list);
    if (sockaddr_smaller(&tmp_remote->addr, addr))
      continue;
    else if (sockaddr_eq(&tmp_remote->addr, addr)) {
      list_for_each(tmp_head2, &tmp_remote->vars_head) {
        tmp_vars = list_entry(tmp_head2, struct vars_struct, vars_list);
        if (tmp_vars->gid < gid)
          continue;
        else if (tmp_vars->gid == gid) {
          memcpy(bestAmongActives, &tmp_vars->bestAmongActives, sizeof(struct bestAmongActives_struct));
          return 0;
        }
        else
          break;
      }
      break;
    }
    else
      break;
  }
  return -1;
}



/* Inserts in variable remotevars all the remote vars received of a particular group.
If the size of the returned list is zero then we return 0. Otherwise
 we return 1. */
inline int get_remote_vars_of_group(u_int gid, struct list_head *remotevars) {
  
  struct list_head *tmp_head1, *tmp_head2;
  
  struct remotevars_struct *tmp_remote;
  struct vars_struct *tmp_vars;
  
  list_for_each(tmp_head1, &remotevars_head) {
    tmp_remote = list_entry(tmp_head1, struct remotevars_struct, remotevars_list);
    list_for_each(tmp_head2, &tmp_remote->vars_head) {
      tmp_vars = list_entry(tmp_head2, struct vars_struct, vars_list);
      if (tmp_vars->gid < gid)
        continue;
      else if (tmp_vars->gid == gid) {
        if (insert_remotevars_in_list(&tmp_remote->addr, tmp_vars->gid, &tmp_vars->accusationTime,
        &tmp_vars->bestAmongActives, NULL, remotevars) < 0)
        fprintf(stderr, "Couldn't allocate new memory when retreiving the remote variables of group: %u\n", gid);
        break;
      }
      else
        break;
    }
  }
  if (list_empty(remotevars))
    return 0;
  else
    return 1;
}



/* Adds or updates variables received from a remote host. */
inline int insert_in_remotevars(struct sockaddr_in *addr, unsigned int gid,
  struct timeval *accusationTime,
  struct bestAmongActives_struct *bestAmongActives,
  struct timeval *bestAmongActives_accTime) {
  
  int retval;
  retval = insert_remotevars_in_list(addr, gid, accusationTime, bestAmongActives,
  bestAmongActives_accTime, &remotevars_head);
  return retval;
}


int free_host_in_remotevars_list(struct sockaddr_in *addr) {
  
  struct list_head *tmp_head;
  struct remotevars_struct *remote_tmp;
  int retval = 0;
  
  list_for_each(tmp_head, &remotevars_head) {
    remote_tmp = list_entry(tmp_head, struct remotevars_struct, remotevars_list);
    if (sockaddr_smaller(&remote_tmp->addr, addr))
      continue;
    else if (sockaddr_eq(addr, &remote_tmp->addr)) {
      list_del(&remote_tmp->remotevars_list);
      list_free(&remote_tmp->vars_head, struct vars_struct, vars_list);
      free(remote_tmp);
      break;
    }
    else {
      retval = -1;
      break;
    }
  }
  return retval;
}

/*****************************************************************************/
/* End of procedures dealing with the remotevars list                        */
/*****************************************************************************/

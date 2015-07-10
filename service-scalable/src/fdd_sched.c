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


/* fdd_sched.c - schedule events for the future - reports,
 suspicions, delayed reports */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "fdd.h"
#include "misc.h"


/* imported stuff */
extern FILE *flog;
extern struct list_head local_procs_list_head ;

LIST_HEAD(event_list_head);

/* add a new event in the list */
static int add_event(int type, struct localproc_struct *lproc,
  struct trust_struct *tproc, struct delay_struct *delay,
  struct host_struct *host,
  struct uint_struct *remote_group,
  struct timeval *tv) {
  struct list_head *tmp       = NULL ;
  struct event_struct *event  = NULL ;
  struct event_struct *event1 = NULL ;
  int retval ;
  
  /* create the event object */
  event = malloc(sizeof(*event)) ;
  retval = -ENOMEM ;
  if(event == NULL)
    goto out ;
  
  /* sets the event's object fields */
  event->type = type ;
  event->lproc = lproc ;
  event->tproc = tproc ;
  event->delay = delay;
  
  event->host = host ;
  event->remote_group = remote_group ;
  
  /* set the event's schedule time */
  memcpy(&event->tv, tv, sizeof(event->tv)) ;
  
  list_for_each(tmp, &event_list_head) {
    event1 = list_entry(tmp, struct event_struct, event_list) ;
    if(!timercmp(&event->tv, &event1->tv, >))
      break ;
  }
  
  list_add_tail(&event->event_list, tmp);
  retval = 0;
  out:
  return retval;
}

/* remove the event associated with the sending of
 an IP multicast message */
static void sched_remove_hello_multicast_event() {
  struct list_head *tmp = NULL ;
  struct event_struct *event = NULL ;
  
  list_for_each(tmp, &event_list_head) {
    event = list_entry(tmp, struct event_struct, event_list) ;
    if(event->type == EVENT_HELLO) {
      tmp = tmp->prev ;
      list_del(&event->event_list) ;
      free(event) ;
      return ;
    }
  }
}

/* removes the report event for the host */
extern void remove_report_event(struct host_struct *host) {
  struct list_head *tmp       = NULL ;
  struct event_struct *event  = NULL ;
  
  list_for_each(tmp, &event_list_head) {
    event = list_entry(tmp, struct event_struct, event_list) ;
    if(event->type == EVENT_REPORT) {
      if (event->host == host) {
        tmp = tmp->prev ;
        list_del(&event->event_list) ;
        free(event) ;
        return ;
      }
    }
  }
}

/* sched_unsched_host - cancel planned suspect events */
/*  remove suspect events for that lproc and host. if host == NULL, remove
 for any host */
extern int sched_unsched_host(struct localproc_struct *lproc,
  struct host_struct *host) {
  struct list_head *tmp       = NULL ;
  struct event_struct *event  = NULL ;
  int retval ;
  
  list_for_each(tmp, &event_list_head) {
    event = list_entry(tmp, struct event_struct, event_list);
    if(event->type != EVENT_SUSPECT || event->lproc != lproc)
      continue ;
    if(host != NULL && event->tproc != NULL &&
    !sockaddr_eq(&host->addr, &event->tproc->host->addr) )
    continue ;
    
    tmp = tmp->prev ;
    list_del(&event->event_list) ;
    free(event) ;
  }
  retval = 0 ;
  
  return retval ;
}

/* remove the event associated with the sending of an INITIAL type
 message from the event list */
extern void remove_initial_ed_event(struct host_struct *host) {
  struct list_head *tmp ;
  struct event_struct *event ;
  list_for_each(tmp, &event_list_head) {
    event = list_entry(tmp, struct event_struct, event_list) ;
    if(event->type == EVENT_INITIAL_ED) {
      if (event->host == host) {
        tmp = tmp->prev ;
        list_del(&event->event_list) ;
        free(event);
        break ;
      }
    }
  }
}

/* schedule the sending of a INITIAL type message to a host */
extern int sched_initial_ed_event(struct host_struct *host,
  struct timeval *now) {
  struct timeval next_sending ;
  unit2timer(INITIAL_MAX_INTERVAL, &next_sending) ;
  timeradd(now, &next_sending, &next_sending) ;
  
  remove_initial_ed_event(host) ;
  if( host->stats.local_initial_finished != FINISHED_YES ||
  host->stats.remote_initial_finished != FINISHED_YES )
  return add_event(EVENT_INITIAL_ED, NULL, NULL, NULL, host, NULL, &next_sending) ;
  else
    return 0 ;
}

/* Added for Omega */
/* Returns true if there is a report event in the event list for a particular
 host concerning a particular group */
extern int exists_report_in_event_list(struct sockaddr_in *raddr, u_int gid) {
  struct event_struct *event;
  struct list_head *tmp_event;
  struct host_struct *host;
  struct list_head *tmp_lproc;
  struct localproc_struct *lproc;
  struct list_head *tmp_groupqos;
  struct groupqos_struct *groupqos;
  
  list_for_each(tmp_event, &event_list_head) {
    event = list_entry(tmp_event, struct event_struct, event_list);
    if (event->type == EVENT_REPORT) {
      host = event->host;
      if ((&host->addr) == NULL)
        continue;
      else if (sockaddr_eq(&host->addr, raddr)) {
        list_for_each(tmp_lproc, &local_procs_list_head) {
          lproc = list_entry(tmp_lproc, struct localproc_struct, local_procs_list);
          list_for_each(tmp_groupqos, &lproc->gqlist_head) {
            groupqos = list_entry(tmp_groupqos, struct groupqos_struct, gqlist);
            if ((groupqos->qos != NULL) && (groupqos->gid == gid))
              return 1;
          }
        }
        return 0; /* There's only one report event per host in the event list */
      }
    }
  }
  return 0;
}

/* sched_report - schedule a report message now! */
/* insert an EVENT_REPORT in the list of reports */
extern int sched_report_now(struct host_struct *host, struct timeval *now) {
  
  remove_report_event(host);
  
  return add_event(EVENT_REPORT, NULL, NULL, NULL, host, NULL,
  now) ;
}


/* sched_report - schedule a report message */
/* insert an EVENT_REPORT in the list of reports */
extern int sched_report(struct host_struct *host) {
  
  struct timeval next_report, now, largest_jointly_group_ts;
  
  remove_report_event(host);
  
  /* If we aren't done with the initial estimation of the quality of the link
   or there are no jointly groups, we schedule the report normally. */
  if ((get_largest_jointly_group_ts(host, &largest_jointly_group_ts) == 0) &&
    (host->stats.local_initial_finished == FINISHED_YES)) {
    
    /* Check if the remote host knows about all the processes in the jointly groups.
    If it does not, we keep sending reports at a fast rate.
     This enables the monitoring to start even in case of msg loss. */
    if (timercmp(&largest_jointly_group_ts, &host->local_largest_group_ts_rcvd, ==) ||
      timercmp(&largest_jointly_group_ts, &host->local_largest_group_ts_rcvd, <)) {
      return add_event(EVENT_REPORT, NULL, NULL, NULL, host, NULL,
      &host->next_report_ts);
    }
    else {
      gettimeofday(&now, NULL);
      
      /* schedule a report in 50 ms */
      unit2timer(50, &next_report);
      timeradd(&now, &next_report, &next_report);
      
      if(timercmp(&next_report, &host->next_report_ts, <)) {
        return add_event(EVENT_REPORT, NULL, NULL, NULL, host, NULL,
        &next_report);
      }
      else
      return add_event(EVENT_REPORT, NULL, NULL, NULL, host, NULL,
      &host->next_report_ts);
    }
  }
  else
  return add_event(EVENT_REPORT, NULL, NULL, NULL, host, NULL,
  &host->next_report_ts);
}


/* sched_report - schedule a report message */
/* insert an EVENT_REPORT in the list of reports delta_t milliseconds after now. */
extern int sched_report_a_bit_later(struct host_struct *host, struct timeval *now,
  u_int delta_t) {
  struct timeval tv;
  
  remove_report_event(host);
  
  unit2timer(delta_t, &tv);
  timeradd(now, &tv, &tv);
  return add_event(EVENT_REPORT, NULL, NULL, NULL, host, NULL,
  &tv) ;
}


/* sched_suspect - schedule a suspect event */
/* use sched_unsched_host() first */
extern int sched_suspect(struct localproc_struct *lproc,
  struct trust_struct *tproc,
  struct timeval *tv) {
  
  return add_event(EVENT_SUSPECT, lproc, tproc, NULL, NULL, NULL, tv);
}


/* schedule for now a new event for the sending of a new IP multicast message */
extern int sched_hello_multicast_now() {
  
  struct timeval when;
  
  timerclear(&when);
  
  sched_remove_hello_multicast_event() ;
  return add_event(EVENT_HELLO, NULL, NULL, NULL, NULL, NULL, &when) ;
}


/* schedule a new event for the sending of a new IP multicast message */
extern int sched_hello_multicast(struct timeval *now_hello, u_int when_hello) {
  
  struct timeval when_hello_time ;
  if( 0 == when_hello ) {
    when_hello = (u_int) (500.0*rand()/(RAND_MAX+1.0));
  }
  
  sched_remove_hello_multicast_event() ;
  
  unit2timer(when_hello, &when_hello_time) ;
  timeradd(&when_hello_time, now_hello, &when_hello_time) ;
  return add_event(EVENT_HELLO, NULL, NULL, NULL, NULL, NULL, &when_hello_time) ;
}

/* remove the suspicion of the appartenence of all groups to a host */
extern void remove_host_suspect_group(struct host_struct *host) {
  
  struct list_head *tmp ;
  struct event_struct *event ;
  
  list_for_each(tmp, &event_list_head) {
    event = list_entry(tmp, struct event_struct, event_list) ;
    if(event->type == EVENT_SUSPECT_GROUP) {
      if (event->host == host) {
        tmp = tmp->prev ;
        list_del(&event->event_list) ;
        free(event) ;
      }
    }
  }
}

/* remove the suspicion of the appartenence of a specified group
 to a host */
static void remove_suspect_remote_group(struct host_struct *host,
  unsigned int remote_gid) {
  struct list_head *tmp ;
  struct event_struct *event ;
  
  list_for_each(tmp, &event_list_head) {
    event = list_entry(tmp, struct event_struct, event_list) ;
    if(event->type == EVENT_SUSPECT_GROUP) {
      if ((event->host == host) &&
        (event->remote_group->val == remote_gid)) {
        tmp = tmp->prev ;
        list_del(&event->event_list) ;
        free(event) ;
        break ;
      }
    }
  }
}

/* schedule a suspicion of the appartenence of a group to a remote host */
extern int sched_suspect_remote_group(struct host_struct *host,
  struct uint_struct *remote_group,
  struct timeval *now) {
  struct timeval suspect_ts ;
  remove_suspect_remote_group(host, remote_group->val) ;
  
  unit2timer(2*max(FDD_GROUP_TDU, HELLO_SENDINT), &suspect_ts) ;
  timeradd(now, &suspect_ts, &suspect_ts) ;
  
  return add_event(EVENT_SUSPECT_GROUP, NULL, NULL, NULL, host,
  remote_group, &suspect_ts) ;
}

/* fd_sched_run - check the event queue and execute events when it's time */
extern void fd_sched_run(struct timeval *timeout,
  struct timeval *now) {
  struct list_head *head      = &event_list_head ;
  struct list_head *tmp       = NULL ;
  struct event_struct *event  = NULL ;
  int type ;
  
  
  /* processing the events from the event list */
  while((tmp = head->next) != head) {
    event = list_entry(tmp, struct event_struct, event_list) ;
    if(timercmp(now, &event->tv, <))
      break ;
    
    type = event->type ;
    event->type = EVENT_NONE ; /* prevent it from being deleted */
    switch(type) {
      case EVENT_REPORT: /* has to send a report event */
        local_send_report_host(event->host, now) ;
      break ;
      
      case EVENT_SUSPECT: /* has to suspect a process */
      if(event->lproc->pid == FDD_PID && event->tproc->pid == FDD_PID &&
        event->tproc->gid && event->tproc->gid->val == FDD_GID) {
        /* suspicion on the host itself */
        remote_host_suspect(event->tproc->host, now) ;
      }
      else /* suspiction of a regular client process */
        local_suspect(event->lproc, event->tproc, now) ;
      break ;
      
      case EVENT_HELLO: /* has to send an IP multicast message */
        send_hello_multicast(now) ;
      sched_hello_multicast(now, HELLO_SENDINT) ;
      break ;
      
      case EVENT_INITIAL_ED: /* has to send a message aimed to determine the initial
         value of the standard deviation */
      send_initial_ed_msg(event->host, FINISHED_UNKNOWN, now, now) ;
      break ;
      
      case EVENT_SUSPECT_GROUP: /* suspicion on the appartenence of a group to a host */
        suspect_remote_group(event->host, event->remote_group) ;
      break ;
    }
    list_del(tmp) ;
    free(event) ;
  } /* end while */
  if(timeout != NULL) {
    if(list_empty(head)) {
      /* make timeout the infinit time */
      timerinf(timeout);
      } else {
      event = list_entry(head->next, struct event_struct, event_list);
      timersub(&event->tv, now, timeout) ;
    }
  } /* end if */
}




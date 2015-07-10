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


/* initial_ed.c - keep statistics of distribution of message
 delays and losses */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "fdd.h"
#include "fdd_stats.h"
#include "misc.h"

extern struct timeval local_epoch ;

/* send a INITIAL type message */
extern void send_initial_ed_msg(struct host_struct *host,
  unsigned int remote_finished,
  struct timeval *remote_sending_ts,
  struct timeval *now) {
  int retval ;
  int one_finished_now = 0 ;
  
  /* lock the FINISHED_YES value for the initial of the remote host */
  if( host->stats.remote_initial_finished == FINISHED_NO && remote_finished == FINISHED_YES ) {
    host->stats.remote_initial_finished = FINISHED_YES ;
    one_finished_now++ ;
  }
  /* lock the FINISHED_YES value for the initial of the local host */
  if( host->stats.local_initial_finished == FINISHED_NO &&
    host->stats.nb_average_delay_msg >= AVERAGE_DELAY_MAX_SAMPLES ) {
    host->stats.local_initial_finished = FINISHED_YES ;
    one_finished_now++ ;
  }
  
  if( one_finished_now && host->stats.local_initial_finished == FINISHED_YES &&
    host->stats.remote_initial_finished == FINISHED_YES ) {
    /* stop the stage of computing the initial value of standard deviation of messages */
    stop_initial(host, now) ;
  }
  else if ((host->stats.local_initial_finished == FINISHED_NO) ||
    (host->stats.remote_initial_finished == FINISHED_NO)) {  /* Used to be an unconditional else */
    char msg[SAFE_MSG_LEN] ;
    char *ptr ;
    /* send an other initial type message */
    host->local_initial_ed_seq++;
    ptr = msg_build_initial_ed(msg, host->local_initial_ed_seq, &local_epoch, host->stats.local_initial_finished,
    now, remote_finished, remote_sending_ts) ;
    retval = comm_send(msg, ptr - msg, &host->addr) ;
    if(retval < 0)
      goto out ;
  }
  /* schedule a new initial type message */
  retval = sched_initial_ed_event(host, now) ;
  
  out:
  return ;
}

/* stop the stage of computing the initial value of standard deviation of messages */
extern void stop_initial(struct host_struct *host, struct timeval *now) {
  
  double pl;
  double floor;
  
  host->stats.remote_initial_finished = FINISHED_YES ;
  /* compute the first value of the standard deviation */
  host->stats.est.e_d = compute_average_delays(&host->stats) ;
  
  /* compute the loss probability */
  pl =
  1.0 - ((double) AVERAGE_DELAY_MAX_SAMPLES) / ((double) host->remote_initial_ed_seq);
  
  /* Since we only received AVERAGE_DELAY_MAX_SAMPLES msgs, we cannot
  estimate loss probabilities less than 1/AVERAGE_DELAY_MAX_SAMPLES.
   Consequently pl is computed so as to be at least 1/AVERAGE_DELAY_MAX_SAMPLES. */
  floor = 1.0 / ((double) AVERAGE_DELAY_MAX_SAMPLES);
  host->stats.est.pl = max(floor, pl);
  
  list_free(&host->stats.average_delay_msg_head, struct average_delay_struct, delay_list) ;
  host->stats.nb_average_delay_msg = 0 ;
  
  recalc_needed_sendint(host, now) ;
  
  sched_report(host) ;
}
/* merge an initial type message */
extern void initial_ed_merge(char *msg, int msg_len, struct sockaddr_in *raddr,
  struct timeval *now) {
  
  unsigned int remote_finished, local_finished ;
  unsigned int initial_ed_seq;
  struct timeval remote_sending_ts, local_sending_ts ;
  struct timeval remote_epoch ;
  struct host_struct *host = NULL ;
  
  
  msg_parse_initial_ed(msg, &initial_ed_seq, &remote_epoch, &remote_finished, &remote_sending_ts,
  &local_finished, &local_sending_ts) ;
  
  host = locate_create_host(raddr, &remote_epoch, 0, now) ;
  if( NULL == host )
    goto out ;
  
  if (host->stats.nb_average_delay_msg < AVERAGE_DELAY_MAX_SAMPLES)
    host->remote_initial_ed_seq = initial_ed_seq;
  
  /* immediatly send back an other initial type message */
  send_initial_ed_msg(host, remote_finished, &remote_sending_ts, now) ;
  /* if not finished yet this stage put the received message in the list */
  if( local_finished == FINISHED_NO && host->stats.local_initial_finished == FINISHED_NO ) {
    struct timeval tv_delay ;
    static unsigned int seq = 1 ;
    timersub(now, &remote_sending_ts, &tv_delay) ;
    merge_average_delay_msg(&host->stats, seq, &remote_sending_ts, 0, now) ;
    seq++ ;
  }
  
  out:
  return ;
}


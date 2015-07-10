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


/* stats.c - keep statistics of distribution of message delays and losses */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <strings.h>

#include "fdd.h"
#include "fdd_stats.h"
#include "misc.h"

#define LN10 2.30258509299405
#define LN2 0.69314718

#ifdef LOG
FILE *flog ;
#endif
/*************************** STATIC STUFF **********************/

#if 0
#ifndef INSTANT_EXPECTED_DELAY_OFF
static void half_time_tv(struct timeval *begin_tv,
  struct timeval *end_tv,
  struct timeval *half_tv) {
  unsigned int units ;
  
  timersub(end_tv, begin_tv, half_tv);
  units = timer2unit(half_tv) ;
  units /= 2 ;
  unit2timer(units, half_tv) ;
  timeradd(begin_tv, half_tv, half_tv) ;
}
#endif
#endif

#ifndef INSTANT_EXPECTED_DELAY_OFF
static void compute_time_0(struct timeval *time_0,
  struct timeval *curent_time,
  unsigned int half_life_units,
  unsigned int nb_backward_intervals) {
  int i = 0 ;
  struct timeval half_life_time ;
  
  unit2timer(half_life_units, &half_life_time) ;
  timerclear(time_0) ;
  for( i = 1 ; i <= nb_backward_intervals ; i++ ) {
    timeradd(time_0, &half_life_time, time_0) ;
  }
  timersub(curent_time, time_0, time_0) ;
}
#endif

#ifndef INSTANT_EXPECTED_DELAY_OFF
static double calculate_weight(unsigned int units,
  unsigned int half_life_units) {
  unsigned int s = 0 ;
  double u = 0.0, u2 = 0.0, u3 =0.0 ;
  unsigned int i = 0 ;
  double pow_s = 1.0, pow_u = 1.0 ;
  
  s = units/half_life_units ;
  u = (double)(units%half_life_units)/half_life_units ;
  
  for( i = 0 ; i < s ; i++) pow_s *= 2 ;
  u = u*LN2 ;
  u2 = u * u ;
  u3 = u2 * u ;
  pow_u = 1 + u + u2/2 + u3/6 ;
  
  return (double)1/(pow_s * pow_u) ;
}
#endif

/*
 ** Records the arrival timestamp of the nearly received message, in the list
 ** in order to compute the loss probability afterwards
 */
static int merge_average_msg(struct stats_struct *stats, unsigned int seq,
  struct timeval *sending_ts,
  struct timeval *arrival_ts) {
  struct list_head *tmp = NULL ;
  struct average_lost_struct *average_lost = NULL ;
  struct average_lost_struct *new_average_lost = NULL ;
  int retval ;
  
  list_for_each(tmp, &stats->average_lost_head) {
    average_lost = list_entry(tmp, struct average_lost_struct, lost_msg_list) ;
    if(greater_than(seq, average_lost->seq))
      break ;
  }
  
  retval = -ENOMEM ;
  new_average_lost = malloc(sizeof(*new_average_lost)) ;
  if(NULL == new_average_lost) {
#ifdef OUTPUT
    fprintf(stdout, "merge_average_msg:%s\n", strerror(errno)) ;
#endif
#ifdef LOG_EXTRA
    fprintf(flog, "merge_average_msg:%s\n", strerror(errno)) ;
#endif
    goto out ;
  }
  
  new_average_lost->seq = seq ; /* put the sequence number */
  memcpy(&new_average_lost->arrival_ts, arrival_ts,
  sizeof(new_average_lost->arrival_ts)) ; /* copy the arrival timestamp */
  list_add_tail(&new_average_lost->lost_msg_list, tmp) ;
  retval = 0 ;
  
  out:
  return retval ;
}

/*************** AVERAGE LOSS PROBABILITY COMPUTATION *****************/
/*
 ** Compute the value of the loss probability right after a messages has been lost
 */
static double compute_average_pl_loss(struct stats_struct *stats,
  unsigned int seq,
  unsigned int prev_seq) {
  double retval ;
  unsigned int total = stats->nb_average_total_msg ;
  
  safety_sub(&total, 1) ; /* increment the total nr. of messages,
   without bypassing the maximum unsigned value */
  retval = (double)stats->nb_average_lost_msg/total ;
  
  return retval ;
}

/*
 ** Adjust the value of the loss probability when the current interval get larger than the
 ** virtual interval (1/loss probability)
 */
static double compute_average_pl_adjust(struct stats_struct *stats) {
  unsigned int total = stats->nb_average_total_msg ;
  unsigned int lost = stats->nb_average_lost_msg ;
  
  safety_add(&total, 1) ;
  safety_add(&lost, 1) ;
  
  return (double) lost/total ;
}
/*************************************************************************/

/*
 ** compute the value of the loss probability
 */
static void recompute_pl(struct stats_struct *stats, unsigned int seq,
  unsigned int prev_seq,
  struct timeval *arrival_ts) {
  unsigned int inc = 0 ;
  
  /* lost messages from the previous received message */
  inc = seq - prev_seq ;
  safety_add(&inc, stats->nb_average_total_msg) ;
  
  if( inc == (unsigned int)(-1) ) {
    stats->nb_average_total_msg /= 2 ;
    stats->nb_average_lost_msg  /= 2 ;
  }
  
  stats->nb_average_lost_msg += seq - prev_seq - 1 ;
  stats->nb_average_total_msg += seq - prev_seq ;
  
#ifdef OUTPUT_EXTRA
  fprintf(stdout, "recompute_pl: inc=%u, seq=%u, prev_seq=%u, lost_msg=%u, total=%u\n",
  inc, seq, prev_seq, stats->nb_average_lost_msg, stats->nb_average_total_msg) ;
#endif
#ifdef LOG_EXTRA
  fprintf(flog, "recompute_pl: inc=%u, seq=%u, prev_seq=%u, lost_msg=%u, total=%u\n",
  inc, seq, prev_seq, stats->nb_average_lost_msg, stats->nb_average_total_msg) ;
#endif
  
  /* probability computation */
  if( seq != prev_seq + 1 ) {  /* messages lost event */
    stats->est.pl = compute_average_pl_loss(stats, seq, prev_seq) ;
    /* adjust the (virtual) last interval */
    if( stats->est.pl )
      stats->last_interval_length = (unsigned int) rint(1.0/stats->est.pl) ;
    else
      stats->last_interval_length = 0 ;
    stats->current_interval_length = 1 ;
  }
  else {
    /* increment the current interval */
    safety_increment(&stats->current_interval_length) ;
    
    /* recompute if the last interval is less than the current interval */
    if(stats->current_interval_length >= stats->last_interval_length) {
      stats->est.pl = compute_average_pl_adjust(stats) ;
      if(stats->est.pl > MAX_LOSS_PROBABILITY)
        stats->est.pl = MAX_LOSS_PROBABILITY ;
    }
  }
#ifdef OUTPUT_EXTRA
  fprintf(stdout, "current_interval_length=%u vs. \"last_interval_length\"=%u\n",
  stats->current_interval_length, stats->last_interval_length) ;
#endif
#ifdef LOG_EXTRA
  fprintf(flog, "current_interval_length=%u vs. \"last_interval_length\"=%u\n",
  stats->current_interval_length, stats->last_interval_length) ;
#endif
}

/*
 ** deal with out of order messages: adjust the value
 ** of the average loss probability
 */
static void average_out_of_order_rep(struct stats_struct *stats) {
  
  if( stats->nb_average_lost_msg > 0 )
    stats->nb_average_lost_msg-- ;
}

/*
 ** delays the computation of the loss probability:
 **  the delay is computed so that there are no more than 1% of DELAYED messages
 **  declared as LOST
 */
static void recompute_pl_late(struct host_struct *host,
  struct timeval *now) {
  struct stats_struct *stats = &host->stats ;
  struct list_head *tmp = NULL ;
  struct average_lost_struct *average_lost = NULL ;
  struct timeval limit_arrival_ts ;
  
  /* the delay */
  unit2timer((u_int)rint(sqrt(stats->est.v_d*(1.0/DEVIATION_ACCURACY-1))),
  &limit_arrival_ts) ;
  
#ifdef OUTPUT_EXTRA
  fprintf(stdout, " limit_arrival_ts=%ums\n", timer2unit(&limit_arrival_ts)) ;
#endif
#ifdef LOG_EXTRA
  fprintf(flog, "limit_arrival_ts=%ums\n", timer2unit(&limit_arrival_ts)) ;
#endif
  
  /* the limit in time, up to which the messages will be processed in order
  to find out the loss probability's value
   */
  timersub(now, &limit_arrival_ts, &limit_arrival_ts) ;
  
#ifdef OUTPUT_EXTRA
  fprintf(stdout, "average_lost_list: ") ;
#endif
#ifdef LOG_EXTRA
  fprintf(flog, "average_lost_list: ") ;
#endif
  list_for_each_reverse(tmp, &stats->average_lost_head) {
    average_lost = list_entry(tmp, struct average_lost_struct, lost_msg_list) ;
#ifdef OUTPUT_EXTRA
    fprintf(stdout, "%u ", average_lost->seq) ;
#endif
#ifdef LOG_EXTRA
    fprintf(flog, "%u ", average_lost->seq) ;
#endif
  }
  
#ifdef OUTPUT_EXTRA
  fprintf(stdout, "\n") ;
#endif
#ifdef LOG_EXTRA
  fprintf(flog, "\n") ;
#endif
  
  /* take all the delayed messages from the list up to the limit
  given by the value of the delay processing
   */
  list_for_each_reverse(tmp, &stats->average_lost_head) {
    average_lost = list_entry(tmp, struct average_lost_struct, lost_msg_list) ;
    if(timercmp(&average_lost->arrival_ts, &limit_arrival_ts, >))
      break ;
    if(greater_than(host->stats.last_average_seq, average_lost->seq)) {
#ifdef OUTPUT
      fprintf(stdout, "AVERAGE_PL_MESSAGE_OUT_OF_ORDER: seq=%u, prev_seq=%u\n",
      average_lost->seq, host->stats.last_average_seq) ;
#endif
#ifdef LOG
      fprintf(flog, "AVERAGE_PL_MESSAGE_OUT_OF_ORDER:seq=%u, prev_seq=%u\n",
      average_lost->seq, host->stats.last_average_seq) ;
#endif
      /* treat the overdelayed messages which have been declared lost.
      just adjust the value of the received messages
       */
      average_out_of_order_rep(stats) ;
    }
    else {
      
      recompute_pl(stats, average_lost->seq, host->stats.last_average_seq,
      &average_lost->arrival_ts) ;
      host->stats.last_average_seq = average_lost->seq ;
    }
    tmp = tmp->next ;
    list_del(&average_lost->lost_msg_list) ;
    free(average_lost) ;
  }
  return ;
}

/************** INSTANT EXPECTED DELAY COMPUTATION ********************/
#ifndef INSTANT_EXPECTED_DELAY_OFF
static void clear_old_history_ed(struct stats_struct *stats,
  struct timeval *arrival_ts) {
  struct list_head *tmp = NULL ;
  struct instant_delay_struct *instant_delay = NULL ;
  
  int delete = 0 ;
  
  struct timeval time_0 ;
  compute_time_0(&time_0, arrival_ts, ED_HALF_LIFE, ED_BACKWARD_INTERVALS) ;
  
  list_for_each(tmp, &stats->instant_delay_msg_head) {
    instant_delay = list_entry(tmp, struct instant_delay_struct,
    instant_delay_list) ;
    if( 1 == delete ) {
      tmp = tmp->prev ;
      list_del(&instant_delay->instant_delay_list) ;
      free(instant_delay) ;
      stats->nb_instant_delay_msg-- ;
      continue ;
    }
    
    if( timercmp(&time_0, &instant_delay->arrival_ts, >)) {
      tmp = tmp->prev ;
      delete = 1 ;
    }
  }
}
#endif


#ifndef INSTANT_EXPECTED_DELAY_OFF
static int merge_instant_delay_msg(struct stats_struct *stats,
  unsigned int delay,
  struct timeval *arrival_ts) {
  struct list_head *tmp = NULL ;
  struct instant_delay_struct *instant_delay = NULL ;
  struct instant_delay_struct *instant_delay_temp = NULL ;
  int retval ;
  
  /* add the new message to the instant delay list */
  instant_delay = malloc(sizeof(*instant_delay)) ;
  retval = -ENOMEM ;
  if( NULL == instant_delay ) {
#ifdef OUTPUT
    fprintf(stdout, "merge_instant_delay_msg: out of memory\n") ;
#endif
    goto out ;
  }
  
  instant_delay->delay = delay ;
  
  memcpy(&instant_delay->arrival_ts, arrival_ts, sizeof(*arrival_ts)) ;
  list_for_each(tmp, &stats->instant_delay_msg_head) {
    instant_delay_temp = list_entry(tmp, struct instant_delay_struct,
    instant_delay_list) ;
    if( timercmp(&instant_delay->arrival_ts, &instant_delay_temp->arrival_ts, >) )
      break ;
  }
  list_add_tail(&instant_delay->instant_delay_list, tmp) ;
  stats->nb_instant_delay_msg++ ;
  
  retval = 0 ;
  out:
  return retval ;
}
#endif


#ifndef INSTANT_EXPECTED_DELAY_OFF
static double compute_instant_delay(struct stats_struct *stats,
  struct timeval *arrival_ts) {
  struct list_head *tmp = NULL ;
  struct instant_delay_struct *instant_delay = NULL ;
  
  struct timeval t ;
  unsigned int units = 0 ;
  double weight = 0.0, sum_weight = 0.0 ;
  double instant_ed = 0.0 ;
  
  list_for_each(tmp, &stats->instant_delay_msg_head) {
    instant_delay = list_entry(tmp, struct instant_delay_struct,
    instant_delay_list) ;
    assert(!timercmp(arrival_ts, &instant_delay->arrival_ts, <)) ;
    
    timersub(arrival_ts, &instant_delay->arrival_ts, &t) ;
    units = timer2unit(&t) ;
    weight = calculate_weight(units, ED_HALF_LIFE) ;
    sum_weight += weight ;
    instant_ed += weight*instant_delay->delay ;
  }
  if( sum_weight )
    instant_ed /= sum_weight ;
  else
    instant_ed = 0.0 ;
  
  return instant_ed ;
}
#endif

#ifndef SYNCH_CLOCKS
/* compute the average value of the sending intervals of the messages received from the
remote host
 */
static void compute_average_sendint(struct stats_struct *stats,
  struct timeval *tv_average_sendint) {
  struct list_head *tmp ;
  struct average_delay_struct *entry ;
  unsigned int n = 0 ;
  struct timeval tv_sendint ;
  
  timerclear(tv_average_sendint) ;
  
  /* add all the sendint values of the list in a time structure */
  list_for_each(tmp, &stats->average_delay_msg_head) {
    entry = list_entry(tmp, struct average_delay_struct, delay_list) ;
    unit2timer(entry->remote_sendint, &tv_sendint) ;
    timeradd(tv_average_sendint, &tv_sendint, tv_average_sendint) ;
    n++ ;
  }
  
  if(!n)
    return ;
  
  /* divide the sum by the number of parts (timer structure)*/
  timerdiv(tv_average_sendint, n) ;
  return ;
}
#endif

#ifndef SYNCH_CLOCKS
/*
 ** recompute the value of the expected arrival time of the next message
 */
extern void recompute_expected_arrival(struct host_struct *host) {
  
  struct list_head *tmp ;
  struct stats_struct *stats = &host->stats ;
  struct average_delay_struct *delay ;
  struct timeval tv0 ;
  
  unsigned int s0 = 0 ;
  unsigned int k ;
  
  struct timeval arrival_ts ;
  struct timeval sum_eta ;
  struct timeval expected_arrival ;
  struct timeval tv_eta ;
  struct timeval tv_average_eta ;
  
  int num_samples = 0 ;
  
  timerclear(&tv0) ;
  timerclear(&sum_eta) ;
  timerclear(&expected_arrival) ;
  
  /* average sendint value */
  compute_average_sendint(stats, &tv_average_eta) ;
#ifdef OUTPUT
  fprintf(stdout, "average_sendint=%ld.%ld ; = %u ms\n",
    tv_average_eta.tv_sec, tv_average_eta.tv_usec,
  timer2unit(&tv_average_eta)) ;
#endif
#ifdef LOG
  fprintf(flog, "average_sendint=%ld.%ld ; = %u ms\n",
    tv_average_eta.tv_sec, tv_average_eta.tv_usec,
  timer2unit(&tv_average_eta)) ;
#endif
  list_for_each_reverse(tmp, &stats->average_delay_msg_head) {
    delay = list_entry(tmp, struct average_delay_struct, delay_list) ;
    
    /* set tv0 the first arrival timestamp in the list
    all the further computations will be relative to it
     but it will not altere the results */
    if( !timerisset(&tv0) ) {
      memcpy(&tv0, &delay->arrival_ts, sizeof(tv0)) ;
      s0 = delay->seq ;
    }
    
    timersub(&delay->arrival_ts, &tv0, &arrival_ts) ;
    
    /* for each lost message consider its sending interval as being
     equal to the average value over all sending intervals */
    for(k = s0 + 1 ; k < delay->seq ; k++) {
      timeradd(&sum_eta, &tv_average_eta, &sum_eta) ;
      printf("Lost %u. Adding the average\n", k) ;
    }
    timersub(&arrival_ts, &sum_eta, &arrival_ts) ;
    timeradd(&expected_arrival, &arrival_ts, &expected_arrival) ;
    
    unit2timer(delay->remote_sendint, &tv_eta) ;
    timeradd(&sum_eta, &tv_eta, &sum_eta) ;
    
    s0 = delay->seq ;
    num_samples++ ;
  }
  
  if(num_samples == 0)
    return ;
  
  timerdiv(&expected_arrival, num_samples) ;
  timeradd(&expected_arrival, &sum_eta, &expected_arrival) ;
  timeradd(&expected_arrival, &tv0, &expected_arrival) ;
  
  memcpy(&stats->expected_arrival_ts, &expected_arrival,
  sizeof(stats->expected_arrival_ts)) ;
#ifdef OUTPUT
  fprintf(stdout, "Computed expected_arrival=%ld.%ld, num_samples=%i\n",
  expected_arrival.tv_sec, expected_arrival.tv_usec, num_samples) ;
#endif
#ifdef LOG
  fprintf(flog, "Computed expected_arrival=%ld.%ld, num_samples=%i\n",
  expected_arrival.tv_sec, expected_arrival.tv_usec, num_samples) ;
#endif
}
#endif

/*************** AVERAGE LOSS PROBABILITY COMPUTATION *****************/
extern int merge_average_delay_msg(struct stats_struct *stats,
  unsigned int seq,
  struct timeval *sending_ts,
  unsigned int remote_sendint,
  struct timeval *arrival_ts) {
  struct list_head *tmp = NULL ;
  struct average_delay_struct *average_delay = NULL ;
  int retval ;
  struct average_delay_struct *entry_ptr = NULL ;
  int entry_exists ;
  
  list_insert_ordered(seq, seq, &stats->average_delay_msg_head,
  struct average_delay_struct, delay_list, >) ;
  retval = - ENOMEM ;
  if(entry_ptr == NULL)
    goto out ;
  
#ifdef SYNCH_CLOCKS
  if(timercmp(arrival_ts, sending_ts, >)) {
    timersub(arrival_ts, sending_ts, &entry_ptr->delay_tv) ;
  }
  else {
    timerclear(&entry_ptr->delay_tv) ;
  }
#else
  timersub(arrival_ts, sending_ts, &entry_ptr->delay_tv) ;
#endif
  
  entry_ptr->remote_sendint = remote_sendint ;
  memcpy(&entry_ptr->arrival_ts, arrival_ts, sizeof(entry_ptr->arrival_ts)) ;
  
  stats->nb_average_delay_msg++ ;
  
  /* remove extra messages from the average delay list */
  list_for_each_reverse(tmp, &stats->average_delay_msg_head) {
    if(stats->nb_average_delay_msg <= AVERAGE_DELAY_MAX_SAMPLES)
      break ;
    average_delay = list_entry(tmp, struct average_delay_struct,
    delay_list) ;
    tmp = tmp->next ;
    list_del(&average_delay->delay_list) ;
    free(average_delay) ;
    stats->nb_average_delay_msg-- ;
  }
  
  retval = 0 ;
  out:
  return retval ;
}

/* compute the standard deviation of message delay */
extern unsigned int compute_average_delays(struct stats_struct *stats) {
  
  unsigned int num_samples ;
  
  struct list_head *tmp = NULL ;
  struct average_delay_struct *average_delay = NULL ;
  
  struct timeval ed_tv ;
  
  unsigned int diff = 0 ;
  struct timeval diff_tv ;
  struct timeval vd_tv ;
  
  timerclear(&ed_tv) ;
  timerclear(&vd_tv) ;
  
  num_samples = 0 ;
  /* first compute the value of the average delay */
  list_for_each(tmp, &stats->average_delay_msg_head) {
    average_delay = list_entry(tmp, struct average_delay_struct, delay_list) ;
    
    timeradd(&ed_tv, &average_delay->delay_tv, &ed_tv) ;
    num_samples++ ;
  }
  
  /* skip the computation if there are too few values */
  if(num_samples < 5 /*AVERAGE_DELAY_MAX_SAMPLES*/)
    return stats->est.e_d;
  
  timerdiv(&ed_tv, num_samples) ;
  
  num_samples = 0 ;
  /* compute the value of the delay variation */
  list_for_each(tmp, &stats->average_delay_msg_head) {
    average_delay = list_entry(tmp, struct average_delay_struct,
    delay_list) ;
    
    timersub(&average_delay->delay_tv, &ed_tv, &diff_tv) ;
    if(diff_tv.tv_sec < 0)
      timerinv(&diff_tv) ;
    
    diff = timer2unit(&diff_tv) ;
    diff *= diff ;
    unit2timer(diff, &diff_tv) ;
    timeradd(&diff_tv, &vd_tv, &vd_tv) ;
    num_samples++ ;
  }
  
  timerdiv(&vd_tv, num_samples) ;
  stats->est.v_d = timer2unit(&vd_tv) ;
  
#ifdef OUTPUT
  printf("Num_samples = %u\n", num_samples) ;
#endif
  
  return timer2unit(&ed_tv) ;
}

/****************** EXTERN STUFF *********************/
/* stats_init - initialize a stats structure */
extern void stats_init(struct stats_struct *stats) {
  
  INIT_LIST_HEAD(&stats->average_lost_head) ;
  
  bzero(&stats->expected_arrival_ts, sizeof(stats->expected_arrival_ts)) ;
  stats->local_initial_finished = FINISHED_NO ;
  stats->remote_initial_finished = FINISHED_NO ;
  stats->last_seq = 0 ;
  
  stats->last_average_seq = 0 ;
  stats->nb_average_total_msg = 0 ;
  stats->nb_average_lost_msg = 0 ;
  
  INIT_LIST_HEAD(&stats->average_delay_msg_head) ;
  stats->nb_average_delay_msg = 0 ;
  stats->nb_average_recompute_period = 1 ;
  
  stats->est.pl = INITIAL_LOSS_PROBABILITY ;
  stats->est.e_d = stats->est.v_d = 0.0 ;
  
  stats->last_interval_length = (unsigned int)rint(1.0/INITIAL_LOSS_PROBABILITY) ;
  stats->current_interval_length = 0 ;
}

/* stats_new_sample - incorporate a new sample and recalculate the stats */
extern void stats_new_sample(struct host_struct *host, unsigned int seq,
  struct timeval *sending_ts,
  struct timeval *arrival_ts,
  unsigned int remote_sendint) {
  
  struct stats_struct *stats = &host->stats ;
  
  /* inits for the first received message */
  if( stats->nb_average_delay_msg == 0 ) {
    host->stats.last_average_seq = seq - 1 ;
  }
  
  /* standard deviation computation */
  recompute_delays(stats, seq, sending_ts,
  remote_sendint, arrival_ts ) ;
#ifndef SYNCH_CLOCKS
  /* expected arrival time computation */
  recompute_expected_arrival(host) ;
#endif
  /* include this message in the loss probability computation */
  merge_average_msg(stats, seq, sending_ts, arrival_ts) ;
  /* recompute the loss probability "latelly" */
  recompute_pl_late(host, arrival_ts) ;
  
#ifdef OUTPUT_EXTRA
  fprintf(stdout, "pl=%f, vd=%f, seq=%u, host=%u.%u.%u.%u, "
    "expected_arrival:%ld.%ld\n"
    "Host's sendint:%u\n",
    host->stats.est.pl, host->stats.est.v_d, seq,
    NIPQUAD(&host->addr),
    host->stats.expected_arrival_ts.tv_sec,
    host->stats.expected_arrival_ts.tv_usec,
  remote_sendint) ;
#endif
#ifdef LOG_EXTRA
  fprintf(flog, "pl=%f, ed=%f, vd=%f, seq=%u, host=%u.%u.%u.%u\n"
    "Host's sendint:%u\n",
    host->stats.est.pl, host->stats.est.e_d, host->stats.est.v_d, seq,
  NIPQUAD(&host->addr), remote_sendint) ;
#endif
}

extern void recompute_delays(struct stats_struct *stats,
  unsigned int seq,
  struct timeval *sending_ts,
  unsigned int remote_sendint,
  struct timeval *arrival_ts ) {
  
  int retval = 0 ;
  double average_ed = 0.0 ;
  double instant_ed = 0.0 ;
  
  /* record the timestamp of the nearly received message for the standard deviation
   computation */
  if( MAX_SENDINT > remote_sendint ) {
    retval = merge_average_delay_msg(stats, seq, sending_ts, remote_sendint,
    arrival_ts) ;
    if( retval < 0 )
      goto out ;
  }
  
#ifndef INSTANT_EXPECTED_DELAY_OFF
  retval = merge_instant_delay_msg(stats, unitdelay(arrival_ts, sending_ts), arrival_ts) ;
  if( retval < 0 )
    goto out ;
  
  clear_old_history_ed(stats, arrival_ts) ;
#endif
  
  /* computing the standard deviation */
  average_ed = compute_average_delays(stats) ;
  
  
#ifndef INSTANT_EXPECTED_DELAY_OFF
  if(remote_sendint < ED_BACKWARD_INTERVALS * ED_HALF_LIFE) {
    instant_ed = compute_instant_delay(stats, arrival_ts) ;
  }
#endif
  
  stats->est.e_d = max(average_ed, instant_ed) ;
  out:
  return ;
}


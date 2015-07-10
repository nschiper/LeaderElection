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


/* fdd_instant.c */
blablabla

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include "fdd.h"
#include "fdd_stats.h"
FILE *flog ;


#ifndef INSTANT_COMPUTING_OFF
static double calculate_weight(unsigned int units,
  unsigned int half_life_units) {
  unsigned int s = 0 ;
  double u = 0.0, u2 = 0.0, u3 =0.0 ;
  unsigned int i = 0 ;
  const double LN2 = log(2) ;
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

#ifndef INSTANT_COMPUTING_OFF
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


#ifndef INSTANT_LOSS_PROBABILITY_OFF
static void inline close_instant_lost_interval_loss(struct stats_struct *stats) {
  
  struct list_head *tmp = NULL ;
  
  list_for_each(tmp, &stats->intervals_head) {
    struct instant_lost_interval_struct *interval = NULL ;
    interval = list_entry(tmp, struct instant_lost_interval_struct,
    interval_list) ;
    interval->good_messages += stats->current_interval_length ;
    break ;
  }
}
#endif

#ifndef INSTANT_LOSS_PROBABILITY_OFF
static void inline merge_instant_lost_interval_loss(struct stats_struct *stats,
  unsigned int seq,
  unsigned int prev_seq,
  struct timeval *arrival_ts) {
  struct instant_lost_interval_struct *interval = NULL ;
  
  interval = malloc(sizeof(*interval)) ;
  if( NULL == interval ) {
    fprintf(flog, "merge_instant_lost_interval_loss: out of memory\n") ;
    goto out ;
  }
  
  interval->lost_messages = seq - prev_seq - 1 ;
  interval->good_messages = 0 ;
  interval->begin_seq = prev_seq + 1 ;
  
  memcpy(&interval->begin_interval_ts, &stats->last_arrival_ts,
  sizeof(interval->begin_interval_ts) ) ;
  half_time_tv(&interval->begin_interval_ts, arrival_ts, &interval->interval_ts) ;
  list_add(&interval->interval_list, &stats->intervals_head) ;
  
  out:
  return ;
}
#endif

static void inline half_time_tv(struct timeval &begin_tv, struct timeval &end_tv,
  struct timeval &half_tv) {
  unsigned int units ;
  
  timersub(end_tv, begin_tv, half_tv);
  units = timer2unit(half_tv) ;
  units /= 2 ;
  unit2timer(units, half_tv) ;
  timeradd(begin_tv, half_tv, half_tv) ;
}

#ifndef INSTANT_LOSS_PROBABILITY_OFF
static void inline merge_instant_lost_interval_adjust(struct stats_struct *stats,
  struct timeval *arrival_ts) {
  struct list_head *tmp = NULL ;
  
  list_for_each(tmp, &stats->intervals_head) {
    struct instant_lost_interval_struct *interval = NULL ;
    
    interval = list_entry(tmp, struct instant_lost_interval_struct,
    interval_list) ;
    half_time_tv(&interval->begin_ts, arrival_ts, &interval->interval_ts) ;
    break ;
  }
}
#endif

#ifndef INSTANT_LOSS_PROBABILITY_OFF
static double compute_instant_pl_loss(struct stats_struct *stats,
  struct timeval *arrival_ts) {
  
  unsigned int i = 1 ;
  unsigned int units = 0 ;
  
  struct list_head *tmp = NULL ;
  struct instant_lost_interval_struct *interval = NULL ;
  
  struct timeval t ;
  struct timeval interval_ts ;
  
  double pl = 0.0 ;
  double weight = 0.0 ;
  double sum_weight = 0.0 ;
  
  unsigned int total_messages ;
  
  list_for_each(tmp, &stats->intervals_head) {
    interval = list_entry(tmp, struct instant_lost_interval_struct,
    interval_list) ;
    if( 1 == i && 1 == interval->lost_messages ) {
      i++ ;
      continue ;
    }
    
    timersub(arrival_ts, &interval->interval_ts, &t) ;
    units = timer2unit(&t) ;
    weight = calculate_weight(units, PL_HALF_LIFE) ;
    sum_weight += weight ;
    
    total_messages = interval->lost_messages ;
    
    if( 1 == i )
      safety_add(&total_messages, 1) ;
    else
      safety_add(&total_messages, interval->good_messages) ;
    
    pl += weight * (double)interval->lost_messages/total_messages ;
    i++ ;
#ifdef DEBUG_LOG
    fprintf(stdout, "For i=%u, good_messages=%u, lost_messages=%u, units=%u, "
      "weight=%f, pl=%f\n", i-1, interval->good_messages, interval->lost_messages, units,
    weight, pl) ;
    fprintf(flog, "For i=%u, good_messages=%u, lost_messages=%u, units=%u, "
      "weight=%f, pl=%f\n", i-1, interval->good_messages, interval->lost_messages, units,
    weight, pl) ;
#endif
  }
  if( sum_weight )
    pl /= sum_weight ;
  else
    pl = 0.0 ;
  return pl ;
}
#endif

#ifndef INSTANT_LOSS_PROBABILITY_OFF
static double compute_instant_pl_adjust(struct stats_struct *stats,
  struct timeval *arrival_ts) {
  
  unsigned int units = 0 ;
  struct list_head *tmp = NULL ;
  struct instant_lost_interval_struct *interval = NULL ;
  
  struct timeval t ;
  struct timeval interval_ts ;
  
  double pl = 0.0 ;
  double weight = 0.0 ;
  double sum_weight = 0.0 ;
#ifdef DEBUG_LOG
  unsigned int i = 1 ;
#endif
  unsigned int total_msg ;
  
  list_for_each(tmp, &stats->intervals_head) {
    interval = list_entry(tmp, struct instant_lost_interval_struct,
    interval_list) ;
    timersub(arrival_ts, &interval->interval_ts, &t) ;
    units = timer2unit(&t) ;
    weight = calculate_weight(units, PL_HALF_LIFE) ;
    sum_weight += weight ;
    
    total_msg = interval->lost_messages ;
    safty_add(&total_msg, interval->good_messages) ;
    safty_add(&total_msg, stats->current_interval_length) ;
    
    pl += weight * (double)(interval->lost_messages)/total_msg ;
    
#ifdef DEBUG_LOG
    i++ ;
    fprintf(stdout, "Adjust: For i=%u, good=%u, lost=%u, units=%u, "
      "weight=%f, pl=%f\n",
      i-1, interval->good_messages, interval->lost_messages, units,
    weight, pl) ;
    fprintf(flog, "Adjust: For i=%u, good=%u, lost=%u, units=%u, "
      "weight=%f, pl=%f\n",
      i-1, interval->good_messages, interval->lost_messages, units,
    weight, pl) ;
#endif
  }
  if( sum_weight )
    pl /= sum_weight ;
  else
    pl = 0.0 ;
  return pl ;
}
#endif


static void inline safety_increment(unsigned int *seq) {
  if(*seq != (unsigned int)(-1))
    (*seq)++ ;
}
static void inline safety_decrement(unsigned int *seq) {
  if(*seq != 0)
    (*seq)-- ;
}
static void inline safety_add(unsigned int *seq, unsigned int offset) {
  if( ((unsigned int)(-1) - (*seq)) > offset )
    (*seq) += offset ;
  else
    (*seq) = (unsigned int)(-1) ;
}
static void inline safety_sub(unsigned int *seq, unsigned int offset) {
  if( (*seq) > offset )
    (*seq) -= offset ;
  else
    (*seq) = 0 ;
}

#ifndef INSTANT_EXPECTED_DELAY_OFF
extern void clear_old_history_ed(struct stats_struct *stats,
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
extern  int merge_instant_delay_msg(struct stats_struct *stats,
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
    fprintf(flog, "merge_instant_delay_msg: out of memory\n") ;
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
extern double compute_instant_delay(struct stats_struct *stats,
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



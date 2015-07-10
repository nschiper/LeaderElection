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

/* fdd.h */
#ifndef _FDD_H
#define _FDD_H

#include <netinet/in.h>
#include <math.h>
#include <limits.h>
#include "fdd_portab.h"
#include "fdd_msg.h"

#define USECS_PER_UNIT 1000	/* all other times in 1000s of usecs
should be multiple of 10 and less than
 1000000 */
#define UNITS_PER_SEC (1000000L / USECS_PER_UNIT)

#define DEFAULT_BADDR 0xffffffff
#define DEFAULT_PORT 4413

#define INITIAL_MAX_INTERVAL 10 /* ten miliseconds */

#define SENDINT_GRAN 50
#define MIN_SENDINT 5
#define MAX_SENDINT (24u * 3600 * UNITS_PER_SEC) /* maximum sendint allowed
 - 24 hours */

#define FDD_GROUP_TDU (10u * 60u * UNITS_PER_SEC) /* ten minutes */
#define FDD_GROUP_TMU (10u * 60u * UNITS_PER_SEC) /* ten minutes */
#define FDD_GROUP_TMRL (10u * 356u * 24u * 3600u) /* ten years  */

#define FDD_PID 0
#define FDD_GID 0

#define HELLO_SENDINT (10 * UNITS_PER_SEC)
#define HELLO_GROUP "225.0.0.37"  /* My group multicat IP */


#define print_ts(tv) printf("[%010ld.%06ld] ", (tv)->tv_sec, (tv)->tv_usec)
#define fprint_ts(stream, tv) fprintf(stream, "[%010ld.%06ld] ", \
(tv)->tv_sec, (tv)->tv_usec)


static inline void timerinf(struct timeval *tv)
{
  /*
#if defined (__OSF__) || defined (__osf__)
  tv->tv_sec = MAX_LONG ;
#else
   */
  tv->tv_sec = LONG_MAX;
  /*
#endif
   */
  tv->tv_usec = 999999L;
}

static inline void timermult(struct timeval *tv, unsigned int n) {
  
  double x = n ;
  x *= tv->tv_usec ;
  tv->tv_sec = tv->tv_sec * n ;
  while( x > 1000000.0 ) {
    x /= 1000000.0 ;
    tv->tv_sec++ ;
  }
  tv->tv_usec = (long)(rint(x));
}

static inline void timerinv(struct timeval *tv) {
  tv->tv_sec = -tv->tv_sec ;
  tv->tv_usec = -tv->tv_usec ;
  while(tv->tv_usec < 0) {
    tv->tv_sec-- ;
    tv->tv_usec += 1000000L ;
  }
}

static inline void timerdiv(struct timeval *tv, unsigned int n) {
  int negative = 0 ;
  
  if(tv->tv_sec < 0) {
    negative = 1 ;
    timerinv(tv) ;
  }
  
  tv->tv_usec = ((tv->tv_sec%n)*1000000L + tv->tv_usec)/n ;
  tv->tv_sec /= n ;
  
  if(negative)
    timerinv(tv) ;
}

static inline void unit2timer(u_int units, struct timeval *tv)
{
  tv->tv_sec = units / UNITS_PER_SEC;
  tv->tv_usec = (units * USECS_PER_UNIT) % 1000000L;
}

static inline u_int timer2unit(struct timeval *tv)
{
  return tv->tv_sec * UNITS_PER_SEC +
  tv->tv_usec / USECS_PER_UNIT + ((tv->tv_usec % USECS_PER_UNIT)? 1 : 0) ;
}

static inline void sockaddr_local(struct sockaddr_in *a)
{
  a->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
}

static inline int sockaddr_islocal(struct sockaddr_in *a)
{
  return a->sin_addr.s_addr == htonl(INADDR_LOOPBACK);
}

/* greater(a,b) <==> (a >= b) */
static inline int  greater_than(unsigned int seq, unsigned int prev_seq) {
  return ( (unsigned int)(seq - prev_seq) < ((unsigned int)(-1))/2 ) ;
}

static inline int between(unsigned int a, unsigned int b, unsigned int c) {
  return (greater_than(b, a) && greater_than(c, b)) ;
}


static inline u_int  unitdelay(struct timeval *now, struct timeval *before) {
  struct timeval diff;
  int d;
  if(timercmp(now, before, <)) {
    d = 0 ;
  }
  else {
    timersub(now, before, &diff);
    d = timer2unit(&diff) ;
  }
  return d;
}


extern struct sockaddr_in fdd_local_addr;
extern int fdd_init();
extern void terminate_fdd();

extern inline int sockaddr_eq(struct sockaddr_in *a, struct sockaddr_in *b);


/**** Communication module ****/
extern int comm_init(struct sockaddr_in *saddr);
extern void comm_cleanup(void);
extern int comm_recv(char *buf, int len, struct sockaddr_in *raddr);
extern int comm_bcast(char *buf, int len);
extern int comm_send(char *buf, int len, struct sockaddr_in *addr);
extern int comm_hello_multicast(char *buf, int len) ;

/***** Fifo module ****/
extern int fifo_init(void);
extern int fifo_reinit(void);
extern void fifo_cleanup(void);

/***** Local module ****/

extern void local_init(void);
extern void send_hello_multicast(struct timeval *now_hello) ;
extern void local_reg(char *msg, fd_set *dset, struct timeval *reg_ts);
extern void local_send_report(struct timeval *tv);
extern void local_sched_report_sooner(u_int sendint, struct timeval *now,
struct host_struct *host) ;
extern void local_untrust_host(struct localproc_struct  *lproc,
  struct host_struct *host,
struct list_head *remove_list);
extern void local_suspect(struct localproc_struct *lproc,
  struct trust_struct *tproc,
struct timeval *trust_ts);
extern void local_change_notify(struct localproc_struct *lproc, int not_type,
  struct trust_struct *tproc,
struct timeval *trust_ts);
extern int insert_ordered(unsigned int val, struct list_head *ord_list) ;
extern int build_groups_list(struct list_head *groups_list) ;
extern struct groupqos_struct *locate_gqos(struct localproc_struct *lproc,
unsigned int gid);
extern struct procqos_struct *locate_pqos(struct localproc_struct *lproc,
  unsigned int pid,
struct sockaddr_in *addr) ;

extern int is_in_proc_list(u_int pid, struct list_head *proc_list) ;
extern int is_in_ordered_proc_list(u_int pid, struct list_head *proc_list) ;
extern int is_in_ordered_list(u_int val, struct list_head *o_list);
extern int join_update_host_jointly_lists(struct host_struct *host, unsigned int gid) ;
extern int proc_trust_list_member(struct localproc_struct *lproc,
  unsigned int pid,
  unsigned int gid,
  unsigned int group_available,
struct sockaddr_in *addr) ;
extern int build_jointly_groups_list(struct host_struct  *host,
struct list_head *all_remote_groups) ;

extern inline int get_group_ts(unsigned int gid, struct timeval *group_ts);
extern int get_largest_jointly_group_ts(struct host_struct *rhost,
struct timeval *largest_jointly_group_ts);

/**** Remote module ****/
extern struct list_head remote_host_list_head;
extern void remote_merge(char *msg, int msg_len, struct sockaddr_in *raddr,
struct timeval *arrival_ts);
extern void remote_replay_host(struct localproc_struct *lproc,
struct host_struct *host, struct timeval *now);
extern void remote_replay_all(struct localproc_struct *lproc, struct timeval *now);
extern int remote_calc_local_sendint(void);
extern void remote_recalc_all_needed_sendint(void);
extern void show_host(struct list_head *mng_hosts, int n) ;
extern void build_mng_host_list(struct list_head *mng_hosts) ;
extern struct host_struct *locate_host(struct sockaddr_in *addr) ;
extern char *parse_proc_list(char *msg, char *ptr_start, int pcount,
struct list_head *list, int *retval) ;
extern void remote_host_suspect(struct host_struct *host, struct timeval *now) ;
extern void suspect_remote_group(struct host_struct *host,
struct uint_struct *group) ;
/**** Schedule module ****/
extern int sched_delay(struct delay_struct *delay, struct timeval *tv);
extern int exists_report_in_event_list(struct sockaddr_in *raddr, u_int gid);
extern int sched_report_now(struct host_struct *host, struct timeval *now);
extern int sched_report(struct host_struct *host);
extern int sched_report_a_bit_later(struct host_struct *host, struct timeval *now,
u_int delta_t);
extern int sched_suspect(struct localproc_struct *lproc, struct trust_struct *tproc,
struct timeval *tv) ;
extern void fd_sched_run(struct timeval *timeout, struct timeval *now);
/*  extern int sched_suspect_host(struct host_struct *host, struct timeval *now) ; */
extern int sched_hello_multicast(struct timeval *now, u_int when_hello) ;
extern int sched_hello_multicast_now();
extern int sched_unsched_host(struct localproc_struct *lproc,
struct host_struct *host);

extern int sched_suspect_remote_group(struct host_struct *host,
  struct uint_struct *remote_group,
struct timeval *now) ;
extern void sched_suspect_host_groups(struct host_struct *host,
struct timeval *now) ;
extern void remove_host_suspect_group(struct host_struct *host) ;

/*extern void sched_remove_hello_multicast_event() ;*/

/**** Wei module ****/
extern u_int wei_sendint(struct qos_struct *qos, struct stats_struct *stats);
extern struct host_struct *locate_create_host(struct sockaddr_in *addr,
  struct timeval *epoch,
  unsigned int seq,
struct timeval *now) ;
extern void recalc_needed_sendint(struct host_struct *rhost,
struct timeval *now) ;
extern struct localproc_struct* get_local_proc(unsigned int pid) ;
extern void local_send_report_host(struct host_struct *host,
struct timeval *sending_ts) ;

extern int add_local_client(struct host_struct *host, struct localproc_struct *lproc) ;

/***** Stats module ****/
extern void stats_init(struct stats_struct *stats) ;
extern void stats_new_sample(struct host_struct *host, unsigned int seq,
  struct timeval *sending_ts,
  struct timeval *arrival_ts,
unsigned int remote_sendint) ;

extern void recompute_delays(struct stats_struct *stats,
  unsigned int seq,
  struct timeval *sending_ts,
  unsigned int remote_sendint,
struct timeval *arrival_ts ) ;
extern unsigned int compute_average_delays(struct stats_struct *stats) ;
extern int merge_average_delay_msg(struct stats_struct *stats,
  unsigned int seq,
  struct timeval *sending_ts,
  unsigned int remote_sendint,
struct timeval *arrival_ts) ;

extern void recompute_pl_out_of_order(struct host_struct *rhost,
  struct timeval *arrival_ts,
  unsigned int seq,
struct timeval *sendint_ts) ;
extern void sched_unsched_say_hello() ;
extern void hello_multicast_merge(char *msg, int msg_len, struct sockaddr_in *raddr,
struct timeval *arrival_ts) ;
extern void remove_report_event(struct host_struct *host) ;

extern void recompute_expected_arrival(struct host_struct *host) ;

extern void initial_ed_merge(char *msg, int msg_len, struct sockaddr_in *raddr,
struct timeval *now) ;
extern void send_initial_ed_msg(struct host_struct *host,
  unsigned int remote_seq,
  struct timeval *remote_sending_ts,
struct timeval *now) ;
extern void remove_initial_ed_event(struct host_struct *host) ;
extern int sched_initial_ed_event(struct host_struct *host,
struct timeval *now) ;
extern void stop_initial(struct host_struct *host, struct timeval *now) ;
/****** Management stuff ******/

extern void show_localprocs() ;

/************* The module of reliable diffusion of groups ********************/

extern void local_init_multicast() ;

/************* The omega Module *********************************************/
extern inline void doUponSuspected(struct sockaddr_in *addr, u_int pid, u_int gid, struct timeval *now);

#endif







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


/*    wei.c - Wei Chen configuration procedure  */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "fdd.h"
#include "misc.h"

#ifdef LOG
FILE *flog ;
#endif

static double f(double eta, double tdu, double v_d, double pl) {
  double prod, tduje2 ;
  int j ;
  
  prod = eta;
  for (j = 1; j <= ceil(tdu/eta) - 1; j++) {
    tduje2 = (tdu-j*eta) * (tdu-j*eta);
    prod *= (v_d + tduje2) / (v_d + pl*tduje2);
  }
  
  return prod;
}

/* wei_sendint - calculate sending interval given QoS and network stats */
extern u_int wei_sendint(struct qos_struct *qos, struct stats_struct *stats) {
  double e_d, v_d, pl;
  double tdu, tmu, tmrl;
  double gamma, eta_max;
  double eta, eta_lower, eta_upper, t, mid;
  
  e_d = stats->est.e_d ;
  v_d = stats->est.v_d ;
  pl = stats->est.pl ;
  
  if( 1.0 == pl ) {
    fprintf(stdout, "Loss Probability of 1. Impossible continuing\n") ;
    return MIN_SENDINT;
  }
  
  tdu = qos->TdU;
  tmu = qos->TmU;
  tmrl = qos->TmrL;
  
#ifdef SYNCH_CLOCKS
  if (tdu < e_d)
    return 0;
  
  tdu -= e_d ;
#endif
  
  gamma = (1-pl) * tdu * tdu / (v_d + tdu * tdu);
  eta_max = min(gamma * tmu, tdu) ;
  
  /* Compute the product */
  t = f(eta_max, tdu, v_d, pl);
  
  /* If the result is greater than tmrl, the eta_max is OK */
  if (t/1000.0 >= tmrl) {
    eta = eta_max;
    goto out;
  }
  
  eta_lower = eta_max;
  while (t/1000.0 < tmrl) {
    eta_lower /= 2;
    if (eta_lower < MIN_SENDINT)
      break;
    t = f(eta_lower, tdu, v_d, pl);
  }
  eta_upper = eta_lower * 2;
  
  if (eta_upper < MIN_SENDINT)
    return MIN_SENDINT;
  
  
  do {
    mid = (eta_lower + eta_upper)/2 ;
    t = f(mid, tdu, v_d, pl) ;
    if(t/1000.0 < tmrl)
      eta_upper = mid ;
    else
      eta_lower = mid ;
  } while ( (eta_upper - eta_lower)/eta_upper >= (double)0.5/100 ) ;
  
  eta = eta_lower ;
  out:
  //return ((u_int)floor(eta / SENDINT_GRAN)) * SENDINT_GRAN;
  return eta ;
}









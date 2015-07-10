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

#ifndef _STATS_H
#define _STATS_H

#define INITIAL_LOSS_PROBABILITY  0.01

#define AVERAGE_DELAY_MAX_SAMPLES 128
#define DELAYS_PERIOD_RECOMPUT    20

#define MAX_LOSS_PROBABILITY 0.9

#define DEVIATION_ACCURACY (1.0/100.0)

#define ED_BACKWARD_INTERVALS 7

#define ED_HALF_LIFE 4*UNITS_PER_SEC

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
#endif






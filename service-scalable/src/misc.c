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


/* misc.c */

#include <netinet/in.h>

/* Returns 1 if a is smaller than b, 0 otherwise */
inline int sockaddr_smaller(struct sockaddr_in *a, struct sockaddr_in *b) {
  return a->sin_addr.s_addr < b->sin_addr.s_addr;
}

inline int sockaddr_eq(struct sockaddr_in *a, struct sockaddr_in *b) {
  return a->sin_addr.s_addr == b->sin_addr.s_addr;
}

/* Returns 1 if a is bigger than b, 0 otherwise */
inline int sockaddr_bigger(struct sockaddr_in *a, struct sockaddr_in *b) {
  return a->sin_addr.s_addr > b->sin_addr.s_addr;
}


inline void timerinf(struct timeval *tv)
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

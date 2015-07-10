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

#ifndef timeradd
#define timeradd(a, b, result)                                                \
do {                                                                        \
  (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
  (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;                          \
  if ((result)->tv_usec >= 1000000)                                         \
  {                                                                       \
    ++(result)->tv_sec;                                                   \
    (result)->tv_usec -= 1000000;                                         \
  }                                                                       \
} while (0)
#endif

#ifndef timersub
#define timersub(a, b, result)                                                \
do {                                                                        \
  (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
  (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
  if ((result)->tv_usec < 0) {                                              \
    --(result)->tv_sec;                                                     \
    (result)->tv_usec += 1000000;                                           \
  }                                                                         \
} while (0)
#endif

#if defined (__OSF__) || defined (__osf__) || defined (hpux) || \
defined (__hpux) || defined (hpux__) || defined (__hpux__)
#define snprintf(a, b, c, d) sprintf(a, c, d)
#endif

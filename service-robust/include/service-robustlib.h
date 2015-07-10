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


/* service-robustlib.h */

#include <netinet/in.h>


#define OMEGA_INTERRUPT_NONE	    0
#define OMEGA_INTERRUPT_ANY_CHANGE	1

#define NOT_CANDIDATE 0
#define CANDIDATE 1

struct omega_proc_struct {
  struct sockaddr_in addr;
  unsigned int pid;
  unsigned int gid;
} ;


extern int omega_register(unsigned int pid);
extern int omega_unregister(int omega_int);

extern int omega_startOmega(int omega_int, unsigned int gid, int candidate, int notif_type,
unsigned int TdU, unsigned int TmU, unsigned int TmrL);
extern int omega_stopOmega(int omega_int, unsigned int gid);

extern int omega_parse_notify(int omega_int, struct omega_proc_struct *leader);
extern int omega_getleader(int omega_int, unsigned int gid, struct omega_proc_struct *leader);
extern int omega_interrupt_any_change(int omega_int, unsigned int gid);
extern int omega_interrupt_none(int omega_int, unsigned int gid);

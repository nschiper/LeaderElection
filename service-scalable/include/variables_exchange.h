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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "list.h"
#include <sys/time.h>


/* A struct containing the variables of a process */
struct vars_struct {
  u_int gid;                    /* The gid of the group */
  struct timeval accusationTime;
  struct timeval startTime;
  struct list_head vars_list;   /* The list of vars_struct */
} ;


/* A struct containing the variable of a remote process */
struct remotevars_struct {
  struct sockaddr_in addr;       /* The adress of the computer thas maintains these vars */
  struct list_head vars_head;    /* The list of vars_struct (one struct per group) */
  struct list_head remotevars_list; /* The list of remotevars_struct */
} ;


/* The list of variables of local processes */
struct list_head localvars_head;

/* The list of variables of remote processes */
struct list_head remotevars_head;


int exchange_vars_init();


/*****************************************************************************/
/* The following procedures are utilities procedures that deal with          */
/* remotevars lists (not the global variable!)                               */
/*****************************************************************************/
inline int insert_remotevars_in_list(struct sockaddr_in *addr, u_int gid, struct timeval *accusationTime,
struct timeval *startTime, struct list_head *remotevars);
void print_remotvars_list(FILE *stream, struct list_head *remotevars_list);
void free_remotevars_list(struct list_head *remotevars_list);


/*****************************************************************************/
/* End of utilities procedures                                               */
/*****************************************************************************/


/*****************************************************************************/
/* The following procedures procedures deal with the localvars list          */
/*****************************************************************************/

inline int create_localvars(unsigned int gid);
inline void remove_localvars(unsigned int gid);
inline int localvars_exist(unsigned int gid);
inline int getlocalvars(unsigned int gid, struct timeval *accusationTime, struct timeval *startTime);
inline int getlocalaccusationTime(unsigned int gid, struct timeval *accusationTime);
inline int getlocalstartTime(unsigned int gid, struct timeval *startTime);
inline int set_local_accusationTime(unsigned int gid);
inline int set_local_startTime(unsigned int gid);

/*****************************************************************************/
/* End of procedures dealing with the localvars list                         */
/*****************************************************************************/


/*****************************************************************************/
/* The following procedures deal with the remotevars list                    */
/*****************************************************************************/

inline int get_accusationTime_of_remoteprocess(struct sockaddr_in *addr, u_int gid,
struct timeval *accusationTime);
inline int get_startTime_of_remoteprocess(struct sockaddr_in *addr, u_int gid,
struct timeval *startTime);
inline int get_remote_vars_of_group(u_int gid, struct list_head *remotevars);
inline int insert_in_remotevars(struct sockaddr_in *addr, unsigned int gid,
struct timeval *accusationTime, struct timeval *startTime);
int free_host_in_remotevars_list(struct sockaddr_in *addr);

/*****************************************************************************/
/* End of procedures dealing with the remotevars list                        */
/*****************************************************************************/




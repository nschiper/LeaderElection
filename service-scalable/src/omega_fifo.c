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


/* omega_fifo.c - registration fifo initilization and cleanup */
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include "omega.h"

/* imported stuff */
extern int omega_fifo_fd;
/* end imported stuff */

#ifdef OMEGA_LOG
extern FILE *omega_log ;
#endif

/* fifo_init - creates and opens the registration fifo */
extern int omega_fifo_init(void) {
  unlink(OMEGA_FIFO_REG) ; /* delete (if any) the reg fifo */
  
  (void)umask(0);
  /* create the reg fifo file */
  if (mkfifo(OMEGA_FIFO_REG, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) == -1) {
#ifdef OMEGA_OUTPUT
    fprintf(stdout, "mkfifo.fifo_init:%s\n", strerror(errno)) ;
#endif
    
#ifdef OMEGA_LOG
    fprintf(omega_log, "mkfifo.fifo_init:%s\n", strerror(errno)) ;
#endif
    return -errno;
  }
  
  /* open the reg fifo file for reading */
  if( (omega_fifo_fd = open(OMEGA_FIFO_REG, O_RDONLY | O_NONBLOCK)) < 0 ) {
#ifdef OMEGA_OUTPUT
    fprintf(stdout, "open.omega_fifo_fd.fifo_init:%s\n", strerror(errno)) ;
#endif
    
#ifdef OMEGA_LOG
    fprintf(omega_log, "open.omega_fifo_fd.fifo_init:%s\n", strerror(errno)) ;
#endif
    
    return -errno;
  }
  return omega_fifo_fd;
}

/* fifo_reinit - prevents the fifo getting into a bad state */
extern int omega_fifo_reinit(void) {
  if( close(omega_fifo_fd) < 0 ) {
#ifdef OMEGA_OUTPUT
    fprintf(stdout, "close.omega_fifo_fd:%s\n", strerror(errno)) ;
#endif
    
#ifdef OMEGA_LOG
    fprintf(omega_log, "close.omega_fifo_fd:%s\n", strerror(errno)) ;
#endif
  }
  
  if( (omega_fifo_fd = open(OMEGA_FIFO_REG, O_RDONLY | O_NONBLOCK)) < 0 ) {
#ifdef OMEGA_OUTPUT
    fprintf(stdout, "open.omega_fifo_fd.fifo_reinit:%s\n", strerror(errno)) ;
#endif
    
#ifdef OMEGA_LOG
    fprintf(omega_log, "open.omega_fifo_fd.fifo_reinit:%s\n", strerror(errno)) ;
#endif
    return -errno;
  }
  return omega_fifo_fd;
}

/* fifo_cleanup closes the registration fifo */
extern void omega_fifo_cleanup(void) {
  if( close(omega_fifo_fd) < 0 ) {
    
#ifdef OMEGA_OUTPUT
    fprintf(stdout, "close. omega_fifo_fd.fifo_cleanup:%s\n", strerror(errno)) ;
#endif
    
#ifdef OMEGA_LOG
    fprintf(omega_log, "close. omega_fifo_fd.fifo_cleanup:%s\n", strerror(errno)) ;
#endif
  }
}




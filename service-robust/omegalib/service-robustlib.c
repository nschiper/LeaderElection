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


/* service-robustlib.c */

#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include "omega.h"
#include "omega2lib.h"
#include "pipe.h"
#include "msg.h"

struct registeredproc_struct {
  int omega_qry_fd;
  int omega_int_fd;
  int omega_cmd_fd;
  struct list_head proc_list;
};


LIST_HEAD(registeredproc_list_head);


/*
 * retrieves the registeredproc structure associated with
 * the given interruption file descriptor
 */
static struct registeredproc_struct *lookup_rproc(int fd) {
  struct list_head *tmp;
  struct registeredproc_struct *rproc;
  
  list_for_each(tmp, &registeredproc_list_head) {
    rproc = list_entry(tmp, struct registeredproc_struct, proc_list);
    
    if (rproc->omega_int_fd == fd)
      return rproc;
  }
  
  return NULL;
}


/* add a registeredproc structure to the registeredproc list */
static void add_rproc(struct registeredproc_struct *rproc) {
  list_add(&rproc->proc_list, &registeredproc_list_head);
}


/* delete a registeredproc structure from the registeredproc list */
static void del_rproc(struct registeredproc_struct *rproc) {
  list_del(&rproc->proc_list);
}

/* reads a result type message from the File Detector Module */
static int omega_wait_for_result(struct registeredproc_struct *rproc,
struct omega_proc_struct *leader)
{
  char msg[OMEGA_FIFO_MSG_LEN];
  int result;
  
  result = read_msg(rproc->omega_qry_fd, msg, OMEGA_FIFO_MSG_LEN);
  
  if (result < 0)
    return result;
  
  if (leader != NULL)
    msg_omega_parse_ext_res(msg, &result, &leader->addr, &leader->pid);
  else
    msg_omega_parse_res(msg, &result);
  
  return result;
}


extern int omega_register(unsigned int pid) {
  struct registeredproc_struct *rproc ;
  char msg[OMEGA_FIFO_MSG_LEN] ;
  char omega_fifo_cmd[OMEGA_MAX_FIFO_LEN] ;
  char omega_fifo_qry[OMEGA_MAX_FIFO_LEN] ;
  char omega_fifo_int[OMEGA_MAX_FIFO_LEN] ;
  int omega_reg_fd, omega_cmd_fd, omega_qry_fd, omega_int_fd ;
  int retval ;
  
  pthread_mutex_lock(&registeredproc_list_mutex);
  
  /* create cmd,qry,int pipes */
  snprintf(omega_fifo_cmd, OMEGA_MAX_FIFO_LEN, OMEGA_FIFO_CMD, pid) ;
  snprintf(omega_fifo_qry, OMEGA_MAX_FIFO_LEN, OMEGA_FIFO_QRY, pid) ;
  snprintf(omega_fifo_int, OMEGA_MAX_FIFO_LEN, OMEGA_FIFO_INT, pid) ;
  
  rproc = malloc(sizeof(*rproc)) ;
  retval = -ENOMEM ;
  if (rproc == NULL)
    goto out ;
  
  umask(0) ;
  
  unlink(omega_fifo_cmd);
  if (mkfifo(omega_fifo_cmd, S_IRUSR | S_IWUSR | S_IRGRP) == -1) {
    perror("") ;
    retval = -errno;
    goto free_rproc;
  }
  
  unlink(omega_fifo_qry);
  if (mkfifo(omega_fifo_qry, S_IRUSR | S_IWUSR | S_IWGRP) == -1) {
    perror("") ;
    retval = -errno;
    goto free_rproc;
  }
  
  unlink(omega_fifo_int);
  if (mkfifo(omega_fifo_int, S_IRUSR | S_IWUSR | S_IWGRP) == -1) {
    perror("") ;
    retval = -errno;
    goto free_rproc;
  }
  
  /* open qry,int pipes before registering */
  omega_qry_fd = open(omega_fifo_qry, O_RDONLY | O_NONBLOCK);
  if (omega_qry_fd == -1) {
    perror("") ;
    retval = -errno;
    goto free_rproc;
  }
  
  /* open non-blocking but we want reads to block later on */
  if (fcntl(omega_qry_fd, F_SETFL, 0) == -1) {
    perror("") ;
    retval = -errno;
    goto free_rproc;
  }
  
  omega_int_fd = open(omega_fifo_int, O_RDONLY | O_NONBLOCK);
  if (omega_int_fd == -1) {
    perror("") ;
    retval = -errno;
    goto close_qry;
  }
  
  if (fcntl(omega_int_fd, F_SETFL, 0) == -1) {
    perror("") ;
    retval = -errno;
    goto close_qry;
  }
  
  /* register */
  omega_reg_fd = open(OMEGA_FIFO_REG, O_WRONLY | O_NONBLOCK);
  if (omega_reg_fd == -1) {
    perror("") ;
    retval = -errno;
    goto close_int;
  }
  
  msg_omega_build_reg(msg, pid);
  retval = write_msg(omega_reg_fd, msg, OMEGA_FIFO_MSG_LEN);
  
  if (retval < 0)
    goto close_reg;
  
  omega_cmd_fd = open(omega_fifo_cmd, O_WRONLY); /* may block until omega opens it */
  if(omega_cmd_fd == -1) {
    perror("") ;
    retval = -errno;
    goto close_reg;
  }
  
  rproc->omega_cmd_fd = omega_cmd_fd;
  rproc->omega_qry_fd = omega_qry_fd;
  rproc->omega_int_fd = omega_int_fd;
  
  retval = omega_wait_for_result(rproc, NULL);
  if (retval < 0)
    goto close_reg;
  
  add_rproc(rproc);
  close(omega_reg_fd);
  retval = omega_int_fd;
  goto out;
  
  close_reg:
  close(omega_reg_fd);
  close_int:
  close(omega_int_fd);
  close_qry:
  close(omega_qry_fd);
  free_rproc:
  free(rproc);
  out:
  unlink(omega_fifo_cmd);
  unlink(omega_fifo_qry);
  unlink(omega_fifo_int);
  pthread_mutex_unlock(&registeredproc_list_mutex);
  return retval;
}


extern int omega_unregister(int omega_int) {
  struct registeredproc_struct *rproc;
  int retval;
  
  pthread_mutex_lock(&registeredproc_list_mutex);
  rproc = lookup_rproc(omega_int);
  retval = -EINVAL;
  if (rproc == NULL)
    goto out;
  
  if (close(rproc->omega_cmd_fd) == -1) {
    retval = -errno;
    goto out;
  }
  
  close(rproc->omega_qry_fd);
  close(rproc->omega_int_fd);
  del_rproc(rproc);
  free(rproc);
  
  retval = 0;
  
  out:
  pthread_mutex_unlock(&registeredproc_list_mutex);
  return retval;
}

extern int omega_startOmega(int omega_int, unsigned int gid, int candidate, int notif_type,
  unsigned int TdU, unsigned int TmU, unsigned int TmrL) {
  struct registeredproc_struct *rproc;
  char msg[OMEGA_FIFO_MSG_LEN];
  int retval;
  
  pthread_mutex_lock(&registeredproc_list_mutex);
  rproc = lookup_rproc(omega_int);
  retval = -EINVAL;
  if (rproc == NULL)
    goto out;
  
  msg_omega_build_startomega(msg, gid, candidate, notif_type, TdU, TmU, TmrL);
  retval = write_msg(rproc->omega_cmd_fd, msg, OMEGA_FIFO_MSG_LEN);
  if (retval < 0) {
    fprintf(stdout, "omega_startOmega: write cmd pipe error\n") ;
    goto out;
  }
  
  retval = omega_wait_for_result(rproc, NULL);
  
  out:
  pthread_mutex_unlock(&registeredproc_list_mutex);
  return retval;
}


extern int omega_stopOmega(int omega_int, unsigned int gid) {
  struct registeredproc_struct *rproc;
  char msg[OMEGA_FIFO_MSG_LEN];
  int retval;
  
  pthread_mutex_lock(&registeredproc_list_mutex);
  rproc = lookup_rproc(omega_int);
  retval = -EINVAL;
  if (rproc == NULL)
    goto out;
  
  msg_omega_build_stopomega(msg, gid);
  retval = write_msg(rproc->omega_cmd_fd, msg, OMEGA_FIFO_MSG_LEN);
  if (retval < 0) {
    fprintf(stdout, "omega_stopOmega: write cmd pipe error\n") ;
    goto out;
  }
  
  retval = omega_wait_for_result(rproc, NULL);
  
  out:
  pthread_mutex_unlock(&registeredproc_list_mutex);
  return retval;
}


extern int omega_parse_notify(int omega_int, struct omega_proc_struct *leader) {
  struct registeredproc_struct *rproc;
  char msg[OMEGA_FIFO_MSG_LEN];
  int retval;
  
  pthread_mutex_lock(&registeredproc_list_mutex);
  rproc = lookup_rproc(omega_int);
  retval = -EINVAL;
  if (rproc == NULL)
    goto out;
  
  retval = read_msg(rproc->omega_int_fd, msg, OMEGA_FIFO_MSG_LEN);
  if (retval < 0)
    goto out;
  
  memset(leader, 0, sizeof(*leader));
  msg_omega_parse_notify(msg, &leader->addr, &leader->pid, &leader->gid);
  
  retval = 0;
  
  out:
  pthread_mutex_unlock(&registeredproc_list_mutex);
  return retval;
}


extern int omega_getleader(int omega_int, unsigned int gid, struct omega_proc_struct *leader) {
  
  struct registeredproc_struct *rproc;
  char msg[OMEGA_FIFO_MSG_LEN];
  int retval;
  
  pthread_mutex_lock(&registeredproc_list_mutex);
  rproc = lookup_rproc(omega_int);
  retval = -EINVAL;
  if (rproc == NULL)
    goto out;
  
  msg_omega_build_getleader(msg, gid);
  retval = write_msg(rproc->omega_cmd_fd, msg, OMEGA_FIFO_MSG_LEN);
  if (retval < 0) {
    fprintf(stdout, "omega_getleader: write cmd pipe error\n") ;
    goto out;
  }
  
  retval = omega_wait_for_result(rproc, leader);
  leader->gid = gid;
  
  /* no leader. */
  if (leader->pid == 0)
    memset(&leader->addr, 0x0, sizeof(struct sockaddr_in));
  
  out:
  pthread_mutex_unlock(&registeredproc_list_mutex);
  return retval;
}


static int omega_interrupt_generic(int omega_int, int notif_type, u_int gid) {
  
  struct registeredproc_struct *rproc;
  char msg[OMEGA_FIFO_MSG_LEN];
  int retval;
  
  pthread_mutex_lock(&registeredproc_list_mutex);
  rproc = lookup_rproc(omega_int);
  retval = -EINVAL;
  if (rproc == NULL)
    goto out;
  
  msg_omega_build_int(msg, notif_type, gid);
  retval = write_msg(rproc->omega_cmd_fd, msg, OMEGA_FIFO_MSG_LEN);
  if (retval < 0) {
    fprintf(stdout, "interrupt_generic: write cmd pipe error\n") ;
    goto out;
  }
  retval = omega_wait_for_result(rproc, NULL);
  
  out:
  pthread_mutex_unlock(&registeredproc_list_mutex);
  return retval;
}

extern int omega_interrupt_any_change(int omega_int, u_int gid) {
  return omega_interrupt_generic(omega_int, OMEGA_INTERRUPT_ANY_CHANGE, gid) ;
}

extern int omega_interrupt_none(int omega_int, u_int gid) {
  return omega_interrupt_generic(omega_int, OMEGA_INTERRUPT_NONE, gid) ;
}


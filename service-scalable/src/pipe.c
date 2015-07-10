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

/* pipe.c */

#include <errno.h>
#include <unistd.h>

/* reads a message from a specified file descriptor */
int read_msg(int fd, char *msg, int len) {
  ssize_t count;
  
  count = read(fd, msg, len);
  if (count == -1)
    return -errno;
  if (count < len)
    return -ENODATA;
  
  return 0;
}

/* writes the given message in the specified file descriptor */
int write_msg(int fd, char *msg, int len) {
  ssize_t count;
  
  count = write(fd, msg, len);
  if (count == -1)
    return -errno;
  if (count < len)
    return -ENOSPC;
  
  return 0;
}

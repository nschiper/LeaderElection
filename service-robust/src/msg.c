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


/* msg.c */
/* procedures used by the fdd and omega */

/* avoid accessing misaligned unsigned int, converts to network byte order */
/* FIXME: this is bogus. the conversion is LSB-host order specific
 and we seem to keep everything 32-bit aligned anyway */

inline unsigned int get32(unsigned char *ptr)
{
  return ((unsigned int)ptr[0] << 24) | ((unsigned int)ptr[1] << 16) |
  ((unsigned int)ptr[2] <<  8) | ((unsigned int)ptr[3]);
}

inline void put32(unsigned char *ptr, unsigned int val)
{
  ptr[0] = val >> 24;
  ptr[1] = val >> 16;
  ptr[2] = val >> 8;
  ptr[3] = val;
}

inline char msg_type(char *msg)
{
  return get32(msg);
}

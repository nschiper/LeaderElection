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


/* list.h */
#ifndef _LIST_H
#define _LIST_H

/* This file comes from the Linux kernel source */

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct list_head {
  struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
  (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)



/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __list_add(struct list_head * new,
  struct list_head * prev,
struct list_head * next)
{
  next->prev = new;
  new->next = next;
  new->prev = prev;
  prev->next = new;
}

/*
 * Insert a new entry after the specified head..
 */
static __inline__ void list_add(struct list_head *new, struct list_head *head)
{
  __list_add(new, head, head->next);
}

/*
 * Insert a new entry before the specified head..
 */
static __inline__ void list_add_tail(struct list_head *new,
struct list_head *head)
{
  __list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __list_del(struct list_head * prev,
struct list_head * next)
{
  next->prev = prev;
  prev->next = next;
}

static __inline__ void list_del(struct list_head *entry)
{
  __list_del(entry->prev, entry->next);
}

static __inline__ int list_empty(struct list_head *head)
{
  return head->next == head;
}


/*
 * Splice in "list" into "head"
 */
static __inline__ void list_splice(struct list_head *list,
struct list_head *head)
{
  struct list_head *first = list->next;
  
  if (first != list) {
    struct list_head *last = list->prev;
    struct list_head *at = head->next;
    
    first->prev = head;
    head->next = first;
    
    last->next = at;
    at->prev = last;
  }
}

/*
 * Swap two lists
 */
static __inline__ void list_swap(struct list_head *list1,
struct list_head *list2)
{
  struct list_head tmp;
  
  INIT_LIST_HEAD(&tmp);
  list_splice(list1, &tmp);
  INIT_LIST_HEAD(list1);
  list_splice(list2, list1);
  INIT_LIST_HEAD(list2);
  list_splice(&tmp, list2);
}


static inline unsigned int list_size(struct list_head *list) {
  int size = -1;
  struct list_head *tmp;
  tmp = list;
  
  do {
    size++;
    tmp = tmp->next;
  } while( tmp != list );
  return ((unsigned int)size);
}


#define list_entry(ptr, type, member) \
( (type*) ( (char *)(ptr) - (unsigned long) (&( ((type*)0) -> member)) ) )

#define list_for_each(pos, head) \
for (pos = (head)->next; pos != (head); pos = pos->next)
#define list_for_each_reverse(pos, head) \
for (pos = (head)->prev ; pos != (head) ; pos = pos->prev)
  
#define list_free(head, type, member) do {		          		\
  while (!list_empty(head)) {					                \
    type *entry = list_entry((head)->next, type, member);	\
    list_del(&entry->member);				                \
    free(entry);						                    \
  }								                            \
} while(0);

#define list_insert_ordered(val, member_val, head, type, member_list, CMP) do {    \
  struct list_head *tmp =  NULL ;                                                \
  entry_exists = 0 ;                                                             \
  list_for_each(tmp, head) {                                                     \
    entry_ptr = list_entry(tmp, type, member_list) ;                          \
    if( (entry_ptr->member_val) CMP (val) )                                   \
      continue ;                                                            \
    if( (entry_ptr->member_val) == (val) )                                    \
      entry_exists = 1 ;                                                    \
    break ;                                                                   \
  }                                                                         \
  if(entry_exists)                                                               \
    break ;                                                                   \
  entry_ptr = malloc(sizeof(type)) ;                                             \
  if( NULL == entry_ptr )                                                        \
    break  ;                                                                  \
  (entry_ptr->member_val) = (val) ;                                              \
  list_add_tail(&(entry_ptr->member_list), tmp) ;                                \
} while(0) ;

#define list_remove_ordered(val, member_val, head, type, member_list, CMP) do {    \
  type *el = NULL ;                                                              \
  struct list_head *tmp = NULL ;                                                 \
  found = 0 ;                                                                    \
  list_for_each(tmp, head) {                                                     \
    el = list_entry(tmp, type, member_list) ;                                 \
    if( (el->member_val) CMP (val) )                                          \
      continue ;                                                            \
    if( (el->member_val) == (val) ) {                                         \
      found = 1 ;                                                           \
      list_del(&el->member_list) ;                                          \
      free(el);                                                             \
    }                                                                     \
    break ;                                                                   \
  }                                                                              \
} while(0) ;

#define free_trust(trust) do {                                                     \
  if((trust)->gid)                                                                 \
    free((trust)->gid) ;                                                          \
  free(trust) ;                                                                    \
  (trust) = NULL ;                                                                 \
} while(0) ;

#define free_gqos(gqos) do {                                                       \
  if((gqos)->qos)                                                                  \
    free((gqos)->qos) ;                                                           \
  free(gqos) ;                                                                     \
  (gqos) = NULL ;                                                                  \
}while(0) ;


#define trust_list_free(head) while( !list_empty(head) ) {	           \
  struct trust_struct *entry ;                                         \
  entry = list_entry((head)->next, struct trust_struct, trust_list);   \
  list_del(&entry->trust_list);				                           \
  free_trust(entry);						                           \
} ;

#define all_trust_lists_free() do {                                                \
  struct list_head *tmp_lproc ;                                                    \
  struct localproc_struct *lproc ;                                                 \
  list_for_each(tmp_lproc, &local_procs_list_head) {                               \
    lproc = list_entry(tmp_lproc, struct localproc_struct, local_procs_list);      \
    trust_list_free(&lproc->tmp_tlist_head);                                       \
  }                                                                                \
} while(0) ;

/* Added for Omega */
/* Macro to delete a process and its list of groups in which it was visible.
 Used in procedure proc_quit_all_groups in local.c */
#define visibility_list_proc_free(pid) do {												  \
  struct list_head *tmp_vis;															  \
  struct pid_gid_visibility_struct *pid_gid;											  \
  list_for_each(tmp_vis, &visibility_list_head) {									  \
    pid_gid = list_entry(tmp_vis, struct pid_gid_visibility_struct, pid_list_head);   \
    if (pid == pid_gid->pid) {														  \
      visibility_list_groups_free(&pid_gid->gid_list_head);						  \
      list_del(&pid_gid->pid_list_head);                                            \
      free(pid_gid);																  \
      break;																		  \
    }																				  \
  }                                                                                    \
} while(0);

/* Added for Omega */
/* Macro to delete all the groups from a particular process */
#define visibility_list_groups_free(head) while( !list_empty(head) ) {	             \
  struct glist_struct *entry ;                                                       \
  entry = list_entry((head)->next, struct glist_struct, gcell);	           			 \
  list_del(&entry->gcell);				                          					 \
  free(entry);						                   							   	 \
  entry = NULL; 																	 \
} ;


/* Added for Omega */
/* Macro to delete a particular group from a particular process */
#define visibility_list_group_free(pid, gid) do {                                                	\
  struct list_head *tmp_visibility;																\
  struct pid_gid_visibility_struct *pid_gid;                                                	\
  struct list_head *tmp_gid;														        	\
  struct glist_struct *gid_struct;	 														    \
  list_for_each(tmp_visibility, &visibility_list_head) {							        	\
    pid_gid = list_entry(tmp_visibility, struct pid_gid_visibility_struct, pid_list_head); 	\
    if (pid_gid->pid == pid) {																\
      list_for_each(tmp_gid, &pid_gid->gid_list_head) {									    \
        gid_struct = list_entry(tmp_gid, struct glist_struct, gcell);    			        \
        if (gid_struct->gid == gid) {													    \
          list_del(tmp_gid);																\
          free(gid_struct);														        \
          break;																			\
        }																				    \
      }																						\
      break;																				    \
    } 																				        \
  }                                                                                             \
} while(0);

#define list_fprint(stream, val, head, type, member_list) do {                     \
  struct list_head *tmp ;                                                          \
  list_for_each(tmp, head) {                                                       \
    type *entry = list_entry(tmp, type, member_list) ;                             \
    fprintf(stream, "%u ", entry->val) ;                                           \
  }                                                                                \
} while(0) ;


#endif






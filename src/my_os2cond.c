/* Copyright (C) Yuri Dario & 2000 MySQL AB
   All the above parties has a full, independent copyright to
   the following code, including the right to use the code in
   any manner without any demands from the other parties.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA */

/*****************************************************************************
** The following is a simple implementation of posix conditions
*****************************************************************************/

#define INCL_DOS
#include <os2.h>

#include <process.h>
#include <strings.h>
#include <sys/timeb.h>

//#define DEBUG

#include "pthread.h"


int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
   APIRET	   rc = 0;

   cond->waiting = -1;
   cond->semaphore = -1;

   /* Warp3 FP29 or Warp4 FP4 or better required */
   rc = DosCreateEventSem( NULL, (PHEV)&cond->semaphore, 0x0800, 0);
   if (rc)
      return ENOMEM;

   cond->waiting=0;

#ifdef DEBUG
   printf( "pthread_cond_init cond->semaphore %x\n", cond->semaphore);
#endif

  return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
   APIRET   rc;

#ifdef DEBUG
   printf( "pthread_cond_destroy cond->semaphore %x\n", cond->semaphore);
#endif

   do {
      rc = DosCloseEventSem(cond->semaphore);
      if (rc == 301) DosPostEventSem(cond->semaphore);
   } while (rc == 301);
   if (rc)
      return EINVAL;

	return 0;
}


int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
   APIRET   rc;
   int	    rval;

   // initialize static semaphores created with PTHREAD_COND_INITIALIZER state.
   if (cond->semaphore == -1)
      pthread_cond_init( cond, NULL);

   rval = 0;
   cond->waiting++;

#ifdef DEBUG
   printf( "pthread_cond_wait cond->semaphore %x, cond->waiting %d\n", cond->semaphore, cond->waiting);
#endif

   if (mutex) pthread_mutex_unlock(mutex);

   rc = DosWaitEventSem(cond->semaphore,SEM_INDEFINITE_WAIT);
   if (rc != 0)
      rval = EINVAL;

   if (mutex) pthread_mutex_lock(mutex);

   cond->waiting--;

   return rval;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
			   struct timespec *abstime)
{
   struct timeb curtime;
   long timeout;
   APIRET   rc;
   int	    rval;

   // initialize static semaphores created with PTHREAD_COND_INITIALIZER state.
   if (cond->semaphore == -1)
      pthread_cond_init( cond, NULL);

   _ftime(&curtime);
   timeout= ((long) (abstime->tv_sec - curtime.time)*1000L +
		    (long)((abstime->tv_nsec/1000) - curtime.millitm)/1000L);
   if (timeout < 0)				/* Some safety */
      timeout = 0L;

   rval = 0;
   cond->waiting++;

#ifdef DEBUG
   printf( "pthread_cond_timedwait cond->semaphore %x, cond->waiting %d\n", cond->semaphore, cond->waiting);
#endif

   if (mutex) pthread_mutex_unlock(mutex);

   rc = DosWaitEventSem(cond->semaphore, timeout);
   if (rc != 0)
      rval = ETIMEDOUT;

   if (mutex) pthread_mutex_lock(mutex);

   cond->waiting--;

   return rval;
}


int pthread_cond_signal(pthread_cond_t *cond)
{
   APIRET   rc;

   // initialize static semaphores created with PTHREAD_COND_INITIALIZER state.
   if (cond->semaphore == -1)
      pthread_cond_init( cond, NULL);

   /* Bring the next thread off the condition queue: */
   rc = DosPostEventSem(cond->semaphore);
   return 0;
}


int pthread_cond_broadcast(pthread_cond_t *cond)
{
   int	    i;
   APIRET   rc;

   // initialize static semaphores created with PTHREAD_COND_INITIALIZER state.
   if (cond->semaphore == -1)
      pthread_cond_init( cond, NULL);

		/*
		 * Enter a loop to bring all threads off the
		 * condition queue:
		 */
   i = cond->waiting;
#ifdef DEBUG
   printf( "pthread_cond_broadcast cond->semaphore %x, cond->waiting %d\n", cond->semaphore, cond->waiting);
#endif

   while (i--) rc = DosPostEventSem(cond->semaphore);

   return 0 ;
}


int pthread_attr_init(pthread_attr_t *connect_att)
{
  connect_att->dwStackSize	= 0;
  connect_att->dwCreatingFlag	= 0;
  connect_att->priority		= 0;
  return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
  if (attr)
    *stacksize = attr->dwStackSize;
  else
    return EINVAL;
  return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
  if (attr)
    attr->dwStackSize = stacksize;
  else
    return EINVAL;
  return 0;
}

int pthread_attr_setprio(pthread_attr_t *connect_att,int priority)
{
  connect_att->priority=priority;
  return 0;
}

int pthread_attr_destroy(pthread_attr_t *connect_att)
{
  bzero( connect_att,sizeof(*connect_att));
  return 0;
}

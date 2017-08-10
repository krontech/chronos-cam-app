/*
 *  Copyright (c) 2010-2011, Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  Contact information for paper mail:
 *  Texas Instruments
 *  Post Office Box 655303
 *  Dallas, Texas 75265
 *  Contact information:
 *  http://www-k.ext.ti.com/sc/technical-support/product-information-centers.htm?
 *  DCMP=TIHomeTracking&HQS=Other+OT+home_d_contact
 *  ============================================================================
 *
 */

#include <pthread.h>
#include <sys/time.h>
#include "semp.h"

/** Initializes the semaphore at a given count
 * 
 * @param semp the semaphore to initialize
 * @param val the initial value of the semaphore
 * 
 */
void semp_init (semp_t *semp, unsigned int count)
{
  pthread_cond_init (&semp->condition, NULL);
  pthread_mutex_init (&semp->mutex, NULL);
  semp->semcount = count;
}

/** Deletes the semaphore
 *  
 * @param semp the semaphore to destroy
 */
void semp_deinit (semp_t *semp)
{
  pthread_cond_destroy (&semp->condition);
  pthread_mutex_destroy (&semp->mutex);
}

/** Decreases the value of the semaphore. Blocks if the semaphore
 * value is null.
 * 
 * @param semp the semaphore to decrease
 */
void semp_pend (semp_t *semp)
{
  pthread_mutex_lock (&semp->mutex);
  while (semp->semcount == 0)
  {
    pthread_cond_wait (&semp->condition, &semp->mutex);
  }
  semp->semcount--;
  pthread_mutex_unlock (&semp->mutex);
}

/** Increases the value of the semaphore
 * 
 * @param semp the semaphore to increase
 */
void semp_post (semp_t *semp)
{
  pthread_mutex_lock (&semp->mutex);
  semp->semcount++;
  pthread_cond_signal (&semp->condition);
  pthread_mutex_unlock (&semp->mutex);
}

/** Reset the value of the semaphore
 * 
 * @param semp the semaphore to reset
 */
void semp_reset (semp_t *semp)
{
  pthread_mutex_lock (&semp->mutex);
  semp->semcount = 0;
  pthread_mutex_unlock (&semp->mutex);
}

/** Wait on the condition.
 * 
 * @param semp the semaphore to wait
 */
void semp_wait (semp_t *semp)
{
  pthread_mutex_lock (&semp->mutex);
  pthread_cond_wait (&semp->condition, &semp->mutex);
  pthread_mutex_unlock (&semp->mutex);
}

/** Signal the condition,if waiting
 * 
 * @param semp the semaphore to signal
 */
void semp_signal (semp_t *semp)
{
  pthread_mutex_lock (&semp->mutex);
  pthread_cond_signal (&semp->condition);
  pthread_mutex_unlock (&semp->mutex);
}

/* Nothing beyond this point */

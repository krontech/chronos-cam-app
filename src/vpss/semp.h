/*
 *  Copyright (c) 2010-2011, Texas Instruments Incorporated

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

#ifndef __PSEM_H__
#define __PSEM_H__

/** The structure contains the semaphore value, mutex and green light flag
 */
typedef struct semp_t
{
  pthread_cond_t condition;
  pthread_mutex_t mutex;
  unsigned int semcount;
} semp_t;

/** Initializes the semaphore at a given count
 * 
 * @param semp the semaphore to initialize
 * 
 * @param val the initial value of the semaphore
 */
void semp_init (semp_t *semp, unsigned int count);

/** Deletes the semaphore
 *  
 * @param semp the semaphore to destroy
 */
void semp_deinit (semp_t *semp);

/** Decreases the value of the semaphore. Blocks if the semaphore
 * value is zero.
 * 
 * @param semp the semaphore to decrease
 */
void semp_pend (semp_t *semp);

/** Increases the value of the semaphore
 * 
 * @param semp the semaphore to increase
 */
void semp_post (semp_t *semp);

/** Reset the value of the semaphore
 * 
 * @param semp the semaphore to reset
 */
void semp_reset (semp_t *semp);

/** Wait on the condition.
 * 
 * @param semp the semaphore to wait
 */
void semp_wait (semp_t *semp);

/** Signal the condition,if waiting
 * 
 * @param semp the semaphore to signal
 */
void semp_signal (semp_t *semp);

#endif

/* Nothing beyond this point */

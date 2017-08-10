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
/*
 *  ======== msgq.h ========
 *  This program demostrates MessageQ on a single processor.
 *
 *  This example has a single server Task that receives requests
 *  and responds back. The server Task receives requests via its
 *  MessageQ queue.
 *
 *  There are 3 clients Tasks. Each client creates its own MessageQ queue. 
 *  Each client opens the server's queue and sends a request. In each
 *  message is the reply queue. This allows the server to determine who
 *  to reply to (instead of doing an explicit MessageQ_open).
 *
 *  Each client allocates one message. This message is also used
 *  by the server to send the response. This is possible because 
 *  the content is the same size. Another option is to have the 
 *  server free the message and allocate a new one. Even another 
 *  option would to use MessageQ_staticMsgInit and not have any heaps.
 *  Please refer to the restrictions with MessageQ_staticMsgInit for
 *  more details.
 *
 *  Initially, each client places its arg0 value into the message 
 *  (e.g. clientTask0's arg0 is 0, etc). When the server receives the
 *  request, it simply adds NUMCLIENTS to the message id. This allows
 *  better validation that the message is being returned to the proper
 *  client.
 *
 *  Each client calls Task_sleep(arg0) to add a little variability 
 *  into the example. The example ends after each client sends NUMMSGS
 *  messages.
 *
 *  The server queue was created statically and the clients dynamically 
 *  to show the functionality. The names for the clients queues are not
 *  needed since no one attempts to open them.
 *
 *  See message.k file for expected output.
 */

#ifndef __MSGQ_H__
#define __MSGQ_H__

#include <ti/ipc/MessageQ.h>

/* Constants for the program */
#define DSPSERVERNAME      "UIAConfigurationServerDsp"
#define VIDEOM3SERVERNAME  "UIAConfigurationServerVideoM3"
#define VPSSM3SERVERNAME   "UIAConfigurationServerVpssM3"

/* Diagnostics control masks*/
#define OMX_DEBUG_OFF    0
#define OMX_DEBUG_LEVEL1 1
#define OMX_DEBUG_LEVEL2 2
#define OMX_DEBUG_LEVEL3 4
#define OMX_DEBUG_LEVEL4 8
#define OMX_DEBUG_LEVEL5 16

#define OMX_MSGQ_SERVERNAME_STRLEN     (50)

#define UIA_CONFIGURE_CMD (0x55)
#define UIA_CONFIGURE_ACK (0x56)
#define NACK              (0xFF)

#define OMX_MSGHEAPID_CORE0              (2)
#define OMX_MSGHEAPID_CORE1              (4)
#define OMX_MSGHEAPID_CORE2              (5)

typedef struct ConfigureUIA
{
  MessageQ_MsgHeader header;
  Int enableAnalysisEvents;
  Int debugLevel;
  Int enableStatusLogger;
} ConfigureUIA;

Void configureUiaLoggerClient (Int coreId, ConfigureUIA *pConfigureUIA);

#endif /* __MSGQ_H__ */

/* Nothing beyond this point */

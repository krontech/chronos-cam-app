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
/*
 *  ======== msgq.c ========
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

#include <stdio.h>
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Types.h>
#include <xdc/runtime/Main.h>
#include <ti/ipc/Ipc.h>
#include <xdc/cfg/global.h>
#include "msgq.h"
#include "app_cfg.h"

/*
 *  ======== clientTask ========
 */
Void configureUiaLoggerClient (Int coreId, ConfigureUIA *pCfgUIA)
{
  MessageQ_Handle messageQ;
  MessageQ_QueueId serverQueue;
  MessageQ_Msg msg;
  Int status;
  Char serverName[OMX_MSGQ_SERVERNAME_STRLEN];
  Int heapId = 0;

  if (coreId == 0)
  {
    strcpy (serverName, DSPSERVERNAME);
    heapId = OMX_MSGHEAPID_CORE0;
  }
  else if (coreId == 1)
  {
    strcpy (serverName, VIDEOM3SERVERNAME);
    heapId = OMX_MSGHEAPID_CORE1;
  }
  else if (coreId == 2)
  {
    strcpy (serverName, VPSSM3SERVERNAME);
    heapId = OMX_MSGHEAPID_CORE2;
  }
  else
  {
    printf ("Invalid Core id - should be 1 or 2");
    return;
  }

  /* 
   *  Create client's MessageQ.
   *
   *  Use 'NULL' for name since no since this queueId is passed by
   *  referene and no one opens it by name. 
   *  Use 'NULL' for params to get default parameters.
   */
  messageQ = MessageQ_create (NULL, NULL);
  if (messageQ == NULL)
  {
    System_abort ("Failed to create MessageQ\n");
  }

  /* Open the server's MessageQ */
  do
  {
    status = MessageQ_open (serverName, &serverQueue);
  } while (status < 0);

  msg = MessageQ_alloc (heapId, sizeof (ConfigureUIA));
  if (msg == NULL)
  {
    System_abort ("MessageQ_alloc failed\n");
  }

  /* Have the remote processor reply to this message queue */
  MessageQ_setReplyQueue (messageQ, msg);

  /* Loop requesting information from the server task */
  printf ("UIAClient is ready to send a UIA configuration command\n");

  /* Server will increment and send back */
  MessageQ_setMsgId (msg, UIA_CONFIGURE_CMD);

  ((ConfigureUIA *) msg)->enableStatusLogger = pCfgUIA->enableStatusLogger;
  ((ConfigureUIA *) msg)->debugLevel = pCfgUIA->debugLevel;
  ((ConfigureUIA *) msg)->enableAnalysisEvents = pCfgUIA->enableAnalysisEvents;

  /* Send the message off */
  status = MessageQ_put (serverQueue, msg);
  if (status < 0)
  {
    MessageQ_free (msg);
    System_abort ("MessageQ_put failed\n");
  }

  /* Wait for the reply... */
  status = MessageQ_get (messageQ, &msg, MessageQ_FOREVER);
  if (status < 0)
  {
    System_abort ("MessageQ_get had an error\n");
  }

  /* Validate the returned message. */
  if (MessageQ_getMsgId (msg) != UIA_CONFIGURE_ACK)
  {
    System_abort ("Unexpected value\n");
  }

  System_printf ("UIAClient received UIA_CONFIGURE_ACK\n");
  
  MessageQ_free(msg);

  status = MessageQ_close (&serverQueue);
  if (status < 0)
  {
    System_abort ("MessageQ_close failed\n");
  }

  status = MessageQ_delete (&messageQ);
  if (status < 0)
  {
    System_abort ("MessageQ_delete failed\n");
  }

  printf ("UIAClient is done sending requests\n");
}

/* Nothing beyond this point */

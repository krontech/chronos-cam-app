/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/
#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include <cstring>
#include <time.h>
#include <QDebug>
#include <unistd.h>
#include <fcntl.h>
#include <sys/statvfs.h>

#include "videoRecord.h"
#include "camera.h"

gboolean
buffer_probe(GstPad *pad, GstBuffer *buffer, gpointer data)
{
	const unsigned int weight = 16;
	VideoRecord *recorder = (VideoRecord *)data;
	struct timespec ts;
	unsigned long long nsec;

	/* Update average FPS statistics. */
	clock_gettime(CLOCK_MONOTONIC, &ts);
	nsec = (ts.tv_sec - recorder->lastFrame.tv_sec) * 1000000000UL;
	nsec += ts.tv_nsec;
	nsec -= recorder->lastFrame.tv_nsec;
	memcpy(&recorder->lastFrame, &ts, sizeof(ts));

	/* EWMA Averaging */
	recorder->frameInterval = ((recorder->frameInterval * (weight - 1)) + nsec) / weight;

	/* DEBUG: Syncing on every frame is a great way to reproduce IO issues, but it degrades throughput. */
	//fsync(recorder->fd);
	return TRUE;
}

gboolean bus_call (GstBus     *bus,
		GstMessage *msg,
		gpointer    data);

Int32 VideoRecord::init(void)
{


}

VideoRecord::VideoRecord()
{
	profile = OMX_H264ENC_PROFILE_HIGH;
	level = OMX_H264ENC_LVL_51;
	i_period = 90;
	force_idr_period = 90;
	encodingPreset = OMX_H264ENC_ENC_PRE_HSMQ;


	running = false;

    recordRunning = false;

	bitsPerPixel = 0.7;
	maxBitrate = 40.0;
	framerate = 60;
	strcpy(filename, "");

	/* Set the default file path, or fall back to the MMC card. */
	int i;
	for (i = 1; i <= 3; i++) {
		sprintf(fileDirectory, "/media/sda%d", i);
		if (path_is_mounted(fileDirectory)) {
			return;
		}
	}
	strcpy(fileDirectory, "/media/mmcblk1p1");
}

VideoRecord::~VideoRecord()
{
}


void VideoRecord::debug()
{
    if (running) {
        frameCount++;
#if 0
        if (!(frameCount % 60)) {
            gint level = 77;
            gint drops = 0;
            g_object_get (G_OBJECT (source), "buffer-level", &level, "dropped-frames", &drops, NULL);
            printf("omx_vfcc: buffer-level=%d dropped-frames=%d\n", level, drops);
        }
#endif
    }
}

gboolean
bus_call (GstBus     *bus,
		GstMessage *msg,
		gpointer    data)
{
	VideoRecord* gstDia = (VideoRecord*)data;

    printf("Message received: %s\n", GST_MESSAGE_TYPE_NAME (msg));

	switch (GST_MESSAGE_TYPE (msg)) {

		case GST_MESSAGE_EOS:

			gstDia->eosFlag = true;
			if(gstDia->eosCallback)
				(*gstDia->eosCallback)(gstDia->eosCallbackArg);	//Call user callback
			break;

		case GST_MESSAGE_ERROR:
			gchar  *debug;
			GError *error;
			
			gst_message_parse_error (msg, &error, &debug);
			
			g_printerr ("Error: %s\n", error->message);
			gstDia->error = true;
			if(gstDia->errorCallback)
				(*gstDia->errorCallback)(gstDia->errorCallbackArg, error->message);
			g_error_free (error);
			
			break;
		case GST_MESSAGE_ASYNC_DONE:
			gstDia->recordRunning = true;
		break;
		default:
			break;
	}
}

double VideoRecord::getFramerate()
{
	return 1000000000.0 / double(frameInterval);
}

bool VideoRecord::endOfStream()
{
	return eosFlag;
}




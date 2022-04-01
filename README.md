# Run-N-Gun Mode
Run-N-Gun mode for Chronos 1.4 & 2.1 is a segmented-buffer record mode that allows the camera to save a segment of video while another segment is being recorded.

*Same* as Segmented mode:
 
In Run-N-Gun mode, the record buffer is split into multiple video segments which are kept in memory. When the recording is started, and with each activation of the trigger (physical trigger switch or input signal only), a new segment is stored in memory. 

*Different* from Segmented mode:

If no video is being saved, the new segment would start being saved immediately after activation of the trigger that ends the segment. Otherwise, the segment would wait to be saved in order.

If ring buffer is enabled (which is by default), new frames are allowed to overwrite the oldest saved segment when the buffer is full of segments. 

But if the segment that will be overwritten by new frames is still being saved, the IO(s) of Record End Trigger would be disabled, which means the current recording segment would not be ended and no new segment would be started even if a trigger from IO is input. The red record button on the top of the camera or the stop button on Main screen can end the current recording segment and the whole video recording.

In Run-N-Gun mode, unused recording time is not returned for future segments to use if a recording is stopped early. 

## Run-N-Gun Process Explanation

[![Slides with animation]](https://drive.google.com/file/d/1XO76vrhFBuV-pFRWxcuNkJys2jCd3SYl/view?usp=sharing) 

## Run-N-Gun Manual

[![Guide for operating a Run-N-Gun recording]](https://drive.google.com/file/d/1gvN6YcH32AUDo9-r2rWQeu0YlR3mQkVL/view?usp=sharing)


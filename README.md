# Run-N-Gun Mode
Run-N-Gun mode for Chronos 1.4 & 2.1 is a new record mode that allows the camera to save a segment of video while another segment is being recorded.

Under Run-N-Gun mode, videos are recorded in a segmented buffer. One Record End Trigger ends a segment and starts the saving of it or puts it into a saving waitlist if another segment is still being saved.

Under Segmented mode, the whole buffer is divided into *x* segments. If *ring buffer* is enabled (which is by default), new segments would overwrite old segments when the number of recording segments exceeds *x*. 

Under Run-N-Gun mode, new segments would be allowed to overwrite the old saved segments when the number of recording segments exceeds *x*.

## Record Mode Selection

Run-N-Gun mode is based on *Segmented* record mode that Chronos 1.4 & 2.1 already have. Therefore, a *Run-N-Gun Mode* check box is added under *Segmented* record mode page in *Record Mode* window.

To use Run-N-Gun mode, select and set *Segmented* record mode, then check *Run-N-Gun Mode* check box.

## Record End Trigger Settings

Run-N-GUn mode requires *Record End Trigger* to end segments. Select IO(s) in Trigger/IO Settings window, set its/their function as *Record End Trigger* and choose settings for IO(s), like *Debounce* and *Pull-up*, and trigger delay as needed.

However, we can't use *Invert* for Run-N-Gun mode for now because trigger needs to be disabled and re-enabled sometimes when overwriting is not allowed and *Invert* setting causes problems in some IO settings and with BNC trigger switch cable.

## Record Mode State on Main Window

A record mode state label is added at the bottom of main window, so users can know what record mode the camera is under without going to check in Record Settings -> Record Mode window.

This label would also show the total number of recorded segments, the current saving segment numbder and trigger status while Run-N-Gun recording is running.

The background of the state label is transparent, so it would not block the view of footages.

## Operation of a Run-N-Gun Recording

After all settings and preparations are done, return to main window. The record mode state label shows *Run-N-Gun* at the bottom of the screen.

Press red recording button or record button on main window UI to start a Run-N-Gun recording.

A trigger state label shows at the bottom of the screen after recording starts. 

* on: allow Record End Trigger to end current recording segment and start the next segment of recording
* off: trigger is disabled, current segment would not be ended even if trigger is input

At this stage, footages are shown on the screen.

Input the first trigger from IO signals or BNC trigger switch. A Run-N-Gun progress label shows between the record mode state label and trigger state label. This progress label presents the total number of recorded segments and current saving segment. The footages go blank while saving is running.

Trigger is disabled (*off*) if the next segment would overwrite an old segment that is still being saved. Wait until trigger is back to *on* and input trigger to end current segment and start next segment or press red recording button or record button in main window UI to end current segment and the whole recording.




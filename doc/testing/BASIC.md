Basic Functionality Test
========================

Power on and Reset to Defaults
------------------------
1. Press the power button for less than one second to boot the camera.
    * Fans spin up to 100% and the Chronos splash screen is displayed.
    * The camera continues to boot in less than 60 seconds.
    * The camApp starts in live display mode.
2. Tap the "Util" menu button.
    * The utility menu is entered, with the main tab selected.
3. Tap the "Settings" tab.
    * The utility menu is displayed, with the settings tab selected.
4. Tap the "Revert to defaults" button. 
    * A popup is displayed prompting the user to confirm this operation.
4. Tap the "Yes" button.
    * A popup is displayed which states the settings have been cleared.
    * The popup also asks the user if they wish to restart the application.
5. Tap the "Yes" button to restart the app.
    * The camApp closes, and a shutdown message is shown.
6. Press the power button and release it within 1 second.
    * The camera should power down in less than 30 seconds.
7. Press the power button and release it within 1 second.
    * Chronos splash screen is displayed and the camera boots to the camApp in live display mode.

---

Verify Default Recording Settings
---------------------------------
1. From the camera main window, tap the "Record Settings" button.
    * The recording settings window is opened.
2. Verify the default recording settings.
    * The default resolution should be set to 1280x1024 at 1057 fps.
    * The default image offset should be centered, with an offset of 0x0 pixels.
    * The default gain should be set to 0 dB with an exposure time of approximately 941 us.
3. Tap the "Record Modes" button.
    * The recording modes window is opened.
4. Verify the default recording mode and length.
    * The default recording mode should be set to "Normal"
    * An 8GB camera should have a recording length should be 4.128 seconds, and 4365 frames.
    * A 16GB camera should have a recording length should be 8.260 seconds, and 8734 frames.
    * A 32GB camera should have a recording length should be 16.524 seconds, and 17472 frames.
5. Tap the "Cancel" button.
    * The recording modes window is closed, and the display returns to the recording settings window.
6. Tap the "Trigger Delay" button.
    * The trigger delay window is opened.
7. Verify the default trigger delay settings.
    * The default pre-record delay should be set to zero seconds and zero frames.
    * The default pre-trigger delay should match the default recording length from step 4.
    * The default post-trigger delay should be set to zero seconds and zero frames.
8. Tap the "Cancel" button.
    * The trigger delay window is closed, and the display returns to the recording settings window.
9. Tap the "Cancel" button.
    * The recording settings window is closed, and the display returns to the camera main window.

---

Verify Default Trigger Settings
-------------------------------
1. From the camera main window, tap the "Trigger/IO Settings" button.
    * The trigger/IO settings window is opened.
2. Verify the default settings for IO 1 (BNC).
    * The analog threshold should be set to 2.50 Volts.
    * The action is set to "Record End Trigger"
    * The "Invert" and "20mA Pullup" checkboxes are selected.
3. Verify the default settings for IO 2.
    * The analog threshold is set to 2.50 Volts.
    * The action is set to "None".
    * No checkboxes are selected.
4. Verify the default settings for Input 3 (Isolated).
    * No checkboxes are selected.
5. Verify default trigger state.
    * The input status for IO1 should be Hi
    * The input status for IO2 should be Lo
    * The input status for In3 should be Lo
9. Tap the "Cancel" button.
    * The trigger/IO settings window is closed, and the display returns to the camera main window.

---

Verify Default Utility Settings
-------------------------------
1. From the camera main window, tap the "Util" button.
    * The Utility settings window is opened, with the main tab selected.
2. Verify the default settings.
    * Ask before discarding unsaved video should be set to "If not reviewed"
    * The Auto record and Auto save checkboxes should be unselected.
    * The Enable Zebras checkbox should be selected.
    * The UI on Left Side checkbox should be unselected.
    * The Upside-down Display should be unselected. 
    * Focus Peaking should be disabled, and the default colour should be green, and the threshold set to "Med"
    * The clock should be within 10 minutes of the current time, and should be counting up in realtime.
3. Select the about tab, and verify the camera configuration.
    * The Camera model should display "Chronos 1.4" and list the correct sensor type and quantity RAM.
    * The Serial number should match the value etched onto the bottom of the case.
    * The Camera application version should list 0.3.0, with an optional suffix denoting the release status (alpha/beta/debug)
    * The Build should show a SHA hash and should *NOT* display a dirty suffix.
    * FPGA version should display 3.6
4. Select the kickstarter tab, and review the contents of the text box for anomalies. The text should not be selectable, and the keyboard should not pop up when the box is touched.
5. Select the factory tab and review that the factory options are hidden.
    * The tab should only display a service password menu and a "remove user calibration button"
6. Type in the factory password and verify that the factory buttons appear:
    * Auto factory cal
    * ADC offset cal
    * Column gain cal
    * Black cal all std res
    * White ref
    * Close application
    * Set serial number
    * Show debug controls (checkbox - deselected by default)
7. Select the main tab, and click the "Close" button.
    * The Utility settings window is closed, and the display returns to the camera main window.

---

Simple Recording Test
---------------------
1. From the main window, with no recorded data, verify that the "Play" button is inactive.
    * Text should be grayed out.
    * No action is performed if tapped.
2. Focus the camera on a subject without the use of the focus aid.
    * Default value of the focus aid checkbox should be deselected.
    * Sharp edges that are in focus should not display any abnormal colouration.
3. Tap the focus aid button to enable the focus aid feature.
    * Sharp edges that are in focus should appear green (the default focus aid colour).
4. Adjust the focus on the camera lens to bring the subject in and out of focus.
    * Focus peaking colouration should appear and disappear as the subject moves in
      and out of focus.
5. Press the red recording button on the top of the camera.
    * The camera should begin recording.
    * The two red LEDs should illuminate (one by the jog wheel and one by the power button).
    * The record button on the main window should change its text to read "Stop"
    * TODO: Should we also disable the other settings buttons while recording?
6. After a short recording has been taken, tap the "Stop" button on the camera main window to end the recording.
    * The camera recording should end.
    * The two LEDs should turn off at the end of recording.
    * The stop button on the main window should change its text to read "Record"
    * The Play button should become active.
7. Focus the camera on a different subject, and tap the "Record" button on the main window.
    * A window should appear prompting the user that they may loose unsaved video in RAM.
8. Tap "Yes" to begin recording.
    * The camera should begin recording.
    * The two red LEDs should illuminate.
    * The record button on the main window should change its text to read "Stop"
9. Shake the camera gently or cause some motion to occur in the scene, and then end the recording by pressing the red recording button.
    * The camera recording should end.
    * The two LEDs should turn off at the end of recording.
    * The stop button on the main window should change its text to read "Record"
    * The Play button should become active.

---

Simple Playback Test
-------------------
This test should immediately proceed after a recording test. For the all of these steps, whenever the
playback window is open, the video should appear smooth and glitch free. If the footage was recorded with
focus peaking and exposure aids (zebra stripes) enabled, the video should not contain any focus peaking
pixels or zebra striping. The position of the playback slider should always match the displayed frame
number and the relative position within the recorded video.

1. Tap the "Play" button to open the playback window.
    * The video should now show the start of your recorded video.
    * The play rate should be set to 60fps.
    * The frame number should be set to 1.
    * The mark in position should be set to the first frame.
    * The mark out position should be set to the last frame.
2. Rotate the jog wheel left and right to seek through the live video.
    * Turning the jog wheel to the right should advance the video one frame for each tick of the wheel.
    * Turning the jog wheel to the left should rewind the video one frame for each tick of the wheel.
    * Pressing the jog wheel and turning it right should advance the video ten frames for each tick.
    * Pressing the jog wheel and turning it left should rewind the video ten frames for each tick.
3. Pressing and holding the left and right play arrows should play and rewind the video at the speed
  given by the play rate.
4. Tapping the up and down arrows changes the play rate by either doubling of halving the rate.
    * The maximum play rate should be limited to 960fps.
    * The minimum play rate should be limited to 4.6fps.
    * The higher the play rate, the faster the recorded scene should be when played back with the left and right arrows.
5. Dragging the slider up and down should seek through the recorded video.
    * Dragging the slider to the top will seek to the end of video.
    * Dragging the slider to the bottom will seek to the start of video.
6. Drag the slider close to the start of video and press the left arrow to begin rewinding.
    * When the start of video is reached, the video should wrap around to the end of the video.
7. Rotate the jog wheel to the right to play forward.
    * When the end of video is reached, the video should wrap around to the start of video.

---

Simple Saving Test
------------------
This test should immediately proceed after a playback test, with an recorded scene that contains some
recognizable action or motion. This test should begin from the playback window with no removable media
present.

1. Drag the playback slider approximately halfway through the recoded video, and tap the Mark Start button.
    * The current frame number should be roughly half of the number of recorded frames.
    * The red line at the side of the playback slider should start at the current position of the slider.
    * The mark start frame should match the current frame number.
2. Tap the up and down arrows to select a playback rate of 120 fps.
3. Press and hold the right arrow button for 3 seconds to play through approximately 360 frames of footage.
4. Tap the Mark End button.
    * The current frame number should be roughly 360 frames greater than the mark start position.
    * The red line at the side of the playback slider should end at the current position of the slider.
    * The mark end frame should match the current frame number.
5. Tap the "Settings" button to open the save settings window.
    * The default format should be set to "H.264" at 60fps with a quality of 0.7 bits per pixel.
    * The H.264 profile dropdown menu should be disabled, with a value of "High"
    * The H.264 level dropwdown menu should be disabled, with a value of "Level 51"
    * The filename text field should be empty.
    * The storage location should be greyed out and display "No storage devices detected"
    * The "Settings" and "Close" buttons on the playback window should become inactive.
6. Insert a USB memory stick into the eSATA/USB combo port.
    * Within approximately 15 seconds, the save location should switch to show "/media/sda1 (Filesystem Name)"
7. Hit close to exit the save settings window.
    * The settings and close buttons should become active again.
8. Hit the "Save" button to begin recording the file to the USB memory stick.
    * The displayed video should go back, and a text overlay reading "Saving..." should appear.
    * The playback position will seek to the Mark start position and begin playing forward.
    * The play rate display should update in realtime to show the recording throughput.
    * The Save button should change its text to read "Abort Save", and all other buttons become disabled.
9. When the video reaches the "mark end", the recording should end and return to playback mode.
    * The text overlay should disappear and the display should return to the saved video.
    * The Abort Save button should change its text to read "Save"
    * All buttons should become active again.
10. As soon as the text overlay disappears, remove the USB memory stick and insert it into a laptop for review.
    * Do *NOT* hit the "Safely Remove" button in the save settings window, or the "Eject" buttons in the Util menu.
    * The saved file should have a name of the form "vid_YYYY-MM-DD_HH-MM-SS.mp4", displaying the time at which
        the file was saved (this may differ from when the video was recorded).
    * When opened with a video player, the video should match the selected excerpt on the camera.
    * The video should play smoothly from start to end without any dropped frames, encoding artifacts, or torn frames.
    * No artifacts of focus peaking or exposure aids (zebra stripes) should be present.
11. Examine the video metadata, which should should show:
    * Dimensions: 1280x1024
    * Codec: H.264 High Profile
    * Framerate: 60 frames per second
    * Bitrate: Approximately 40Mbps
    * File Size: Approximately 30MB

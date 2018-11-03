Video Recording Tests
=====================
These tests attempt to validate the performance and feature set of the video encoder system,
including tolerance of low-quality media and use of the advanced encoding formats. These tests
begin after the user has recorded a scene capturing some visible activity. Unless otherwise
specified, recordings should be taken at 1280x1024 and a speed of 1057fps.

H.264 Feature Tests
-------------------
1. From the playback window, select an approximately 360-frame region of the video to be recorded
    using the "Mark Start" and "Mark End" buttons, then tap the Settings button to open the save
    settings window.
2. Set the save format to "H.264" and set the framerate to 60fps.
    * The default quality should be set to 0.7 bits per pixel and 40Mbps.
    * The text should read "40.00Mbps @ 1280x1024 60fps".
3. Tap the down arrows in the bits per pixel box to decrease the quality settings.
    * The bpp should decrese in steps of 0.1, down to a value of zero.
    * The text should read "40.00Mbps" until a value of 0.5bpp is reached, and then
        decrease linearly with the bpp setting.
4. Select a value of 0.3bpp, and then tap the up and down arrows of the "max bitrate" box to adjust the bitrate limit.
    * When the maximum bitrate exceeds 23.59Mbps, the text should read "23.59Mbps @ 1280x1024 60fps"
    * When the maximum bitrate is less than 23.59Mbps, the text should display the selected maximum.
    * The maximum bitrate should be limited to 60Mbps.
5. Select a maximum bitrate of 30Mbps, and then tap the up and down arrows "saved file framerate" box to adjust the framerate.
    * When the framerate reaches 50fps, the text should display "19.66Mbps @ 1280x1024 50fps"
    * When the framerate is decreased below 50fps, the displayed framerate should scale down linearly with fps.
6. Select a framerate of 30fps.
    * The text should now display "11.80Mbps @ 1280x1024 30fps"
7. Tap the "close" button to exit the save settings window, and then hit "Save" to record the
    selected video to the storage device.
8. Remove the storage device from the camera, and insert it into a laptop for review.
    * The video file should play smoothly with no artifacts or frame tearing.
    * The video file should play back at 30fps.
    * The video file should be approximately 17.6MB in size.
    * The video file metadata should list a bitrate of approximately 11.8Mbps.

---

H.264 Throttling Tests
----------------------
1. From the playback window, select a video region of interest of approximately 2000 frames to be recorded
    using the "Mark Start" and "Mark End" buttons, then tap the Settings button to open the save settings
    window.
2. Insert a slow media device such as a non-class 10 SD card, or a cheap USB 2.0 memory stick (recommend the
    16GB Verbatim Store-N-Go with artsy drawings on it).
3. Configure the encoder as follows:
    * H.264 save format
    * 2.00 bits per pixel
    * 60fps saved file framerate
    * 60Mbps maximum bitrate
4. Log into the camera via SSH using the command `ssh -oKexAlgorithms=+diffie-hellman-group1-sha1 root@192.168.12.1`
5. Tap the "Close" button to exit the save settings window, and then tap the "Save" button to start saving.
    * The play rate should fluctuate as the video is being recorded, with an average value of 30-40fps.
6. While saving is in progress, execute the command `dd if=/dev/zero of=/media/sda1/zero.bin bs=4k` over SSH.
    * This should saturate the SD card with memory writes causing the saved frame rate to drop significantly or stop altogether.
    * The play rate may become jerky and inconsistent.
7. After a few seconds, hit `Ctrl-C` to terminate the `dd` command.
    * Video recording should return to normal after roughly 30 seconds and then save as normal.
8. Wait for the recording to complete and then remove the storage device from the camera, and insert it into a laptop for review.
    * The video file should play smoothly with no dropped frames, compression artifacts or frame tearing.

---

Raw Recording Test
------------------
1. From the playback window, select a video region of interest of approximately 360 frames to be recorded
    using the "Mark Start" and "Mark End" buttons, then tap the Settings button to open the save settings
    window.
2. Insert a fast media device such as a USB3.0 flash drive or a class-10 SD card, and select the "Raw 16-bit"
    save format.
    * All other encoder options should be disabled.
    * Select the desired save location on the fast storage device.
    * The text should display "1258.29Mbps @ 1280x1024 60fps"
3. Tap the "Close" button to exit the save settings window, and then the "Save" button to begin recording.
    * The play rate may fluctuate, but should saturate at approximately 2-3fps depending on your storage media.
    * Get a coffee and wait for recording to complete.
4. TODO: Rinse and repeat with "Raw 12-bit packed" format.
4. TODO: Need documentation on how to verify the contents of raw video files.

---

Abort Recording Test
--------------------
1. From the playback window, select a video select the entire video to be recorded using the "Mark Start" and
    "Mark End" buttons, ensuring that this consists of at least 4000 frames, then tap the Settings button to
    open the save settings window.
2. Insert a fast media device and select the H.264 encoding format, at 0.7 bits per pixel, max bitrate of 40 Mbps and 60fps.
3. Tap the "Close" button to exit the save settings window, and then the "Save" button to begin recording.
    * The playback speed should average around 60fps on a fast storage medium.
    * The Save button should have its text replaced with "Abort Save"
4. When the frame position reaches approximately 1000 frames (this will take about 16 seconds from hitting save),
    tap the "Abort Save" button.
    * Uppon pressing "Abort Save" the playback slider will stop, and the frame numbers will cease increasing.
    * After about 15 seconds, the save should terminate and the "Saving" text will be cleared.
    * The video should return to playback mode.
5. As soon as the "Saving" text is cleared, remove the storage media from the camera, and insert it into a
    laptop for review.
    * Do *NOT* hit the "Safely Remove" button on the save settings window, or the Eject buttons on the Util window.
    * The video file should play smoothly with no dropped frames, compression artifacts or frame tearing.
    * The metadata should show:
        - Dimensions: 1280x1024
        - Codec: H.264 High Profile
        - Framerate: 60fps
        - Duration: approximately 17 seconds
        - Bitrate: approximately 40Mbps
        - File Size: 85MB

---

Video Throughput Test
---------------------
1. Record a scene using a resolution of 640x120. Select a region of at least 1000 frames to be recorded,
and save the file to a fast storage medium using H.264 encoding.
    * The play rate should average close to 200fps while saving the video file.
2. Record a scene using a resolution of 640x480. Select a region of at least 1000 frames to be recorded,
and save the file to a fast storage medium using H.264 encoding.
    * The play rate should average close to 125fps while saving the video file.
3. Record a scene using a resolution of 1280x1024. Select a region of at least 1000 frames to be recorded,
and save the file to a fast storage medium using H.264 encoding.
    * The play rate should average close to 60fps while saving the video file.

FAT32 Filesystem Limits
----------------
TODO: Instructions to open the video file in a raw editing program.

1. Ensure that the SD card has been formatted as FAT32 and at least 8GB of free space before starting this test.
2. Record a scene at a resolution of 1280x1024, capturing at least 2000 frames. Select a region of 1700 frames
using the Mark Start and Mark End buttons.
2. Tap the 'Settings' button to open the Save settings window. Select "Raw 16bit" as the save format and tap
the close button to exit the save settings window. This should yeild a file size of approximately 4.15GiB, which
will exceed the FAT32 file size limitation.
3. Tap the 'Save' button to try and save the file.
    * A dialog box should pop up warning the user that the file size is bigger than 4GB.
4. Tap the "No" button to close the pop up warning.
    * The camera should not attempt to save the video file.
5. Tap the 'Settings' button to open the Save settings window. Select "Raw 12bit packed" as the save format and
tap the close button to exit the save settings window. This should yeild a file size of approximately 3.11GiB,
which should be supported by FAT32.
6. Tap the 'Save' button to try and save the file.
    * The camera should begin recording the file to the SD card. Depending on the performance of the card, the
    camera should average around 4-5fps while saving the video file.
7. Wait for the video to complete saving, and then select a new video region of approximately 1600 frames in
length using the Mark Start and Mark End buttons.
8. Tap the 'Settings' button to open the Save settings window. Select "Raw 16bit" as the save format and
tap the close button to exit the save settings window. This should yeild a file size of approximately 3.9GiB,
which should be supported by FAT32.
9. Tap the 'Save' button to try and save the file.
    * The camera should begin recording the file to the SD card. Depending on the perofrmance of the card, the
    camera should average around 3-4fps while saving the video file.
10. Wait for the video to complete saving, remove the SD card and insert it into a laptop for review.
    * The 12-bit packed raw file should be approximately 3.11GiB in size.
    * The 16-bit raw file should be approximately 3.90GiB in size.

---

Free Space Check
----------------
1. Ensure that the SD card has been formatted as FAT32 and has less than 4GB of free space before starting this test.
2. Log into the camera via SSH and calculate the free space on the SD card using the command 'df /media/mmcblk1p1/'.
This will list the available space on the SD card in 1kiB blocks. Take this number and compute the maximum file size
in frames from the formula `frames = blocks / 2560`
3. Record a scene at a resolution of 1280x1024, capturing at least 2000 frames, and use the Mark Start and Mark End
buttons to select a region equal to the calculated number of frames minus one. For example, if the available 1kiB
blocks were 1000000, we would select a Mark Start position of 1, and a Mark End of 389.
4. Tap the 'Settings' button to open the Save settings window. Select "Raw 16bit" as the save format and tap
the close button to exit the save settings window. This should yeild a file size that just barely exceeds the free
space left on the filesystem, as the camapp adds 2 extra frames to the estimate because extra frames are usually saved.
5. Tap the 'Save' button to try and save the file.
    * The camera should display a pop up warning the user that the file size is bigger than the free space available.
6. Tap the "No" button to close the pop up warning.
    * The camera should not attempt to save the video file.
7. Tap the 'Save' button to try and save the file.
    * The camera should display a pop up warning the user that the file size is bigger than the free space available.
8. Tap the "Yes" button to save anyway.
    * The camera should abort the save just before the end of the marked region. 
9. Set the marked region to be one frame long and tap the 'save' button again. 
    * The camera should complain that a video cannot be saved due to lack of free space. Tap the "Yes" button.
10. As soon as the "Saving" text is cleared, remove the storage media from the camera, and insert it into a
    laptop for review.
    * Do *NOT* hit the "Safely Remove" button on the save settings window, or the Eject buttons on the Util window.
    * The amount of free space remaining on the SD card should be less than 20MB, but more than 0.
11. Delete the video from the SD card and reinsert it into the camera.
12. Decrease the mark out position by two frames and tap the 'Mark End' button. This should now give a file size that
is just slightly smaller than the available space on the SD card.
13. Tap the 'Save' button to try and save the file.
    * The camera should begin recording the file to the SD card. Depending on the performance of the card, the
    camera should average around 3-4fps while saving the video file.
14. Wait for the video to complete saving, remove the SD card and insert it into a laptop for review.
    * There should be less than 2MB of free space remaining on the SD card.

---

Ext3 Filesystem Support
-------------------------
1. Using a Linux PC, format the SD card with an ext3 filesystem using the command `sudo mkfs.ext3 -L test-ext3 device`
2. Record a scene at a resolution of 1280x1024, capturing at least 2000 frames, and use the Mark Start and Mark End
buttons to select a region of 1700 frames.
3. Tap the 'Settings' button to open the Save settings window. Select "Raw 16bit" as the save format and tap
the close button to exit the save settings window.
4. Tap the 'Save' button to try and save the file.
    * The camera should begin recording the file to the SD card. Depending on the performance of the card, the
    camera should average around 3-4fps while saving the video file.
5. Wait for the video to complete saving, remove the SD card and insert it into a laptop for review.
    * The expected raw video file should have a size of 4.15GiB
    * The file should be saved with an owner and group of root:root, and permissions set to 644 (-rw-r--r--)

---

Check For Mounted Filesystem
----------------------------
1. Record some footage, and select a region of interest to be saved using the Mark Start and Mark End buttons.
2. Tap the 'Settings' button to open the Save settings window. Select the SD card as the save location and tap
the close button to exit the save settings window.
3. Remove the SD card from the camera, and tap the 'Save' button to try and record the video file.
    * The camera should display a pop up warning that the save location is not found.
4. Tap 'OK' to close the pop up warning.
    * The camera should not save the video file.

---

eSATA Interface
---------------
TODO: Placeholder for more advanced storage tests

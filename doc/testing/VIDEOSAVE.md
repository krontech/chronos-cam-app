Video Recording Tests
=====================
These tests attempt to validate the performance and feature set of the video encoder system,
including tolerance of low-quality media and use of the advanced encoding formats. These tests
begin after the user has recorded a scene capturing some visible activity. Unless otherwise
specified, recordings should be taken at 1280x1024 and a speed of 1057fps.

H.264 Feature Tests
-------------------
1. From the payback window, select an approximately 360-frame region of the video to be recorded
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
5. Select a maximum bitrate of 20Mbps, and then tap the up and down arrows "saved file framerate" box to adjust the framerate.
    * When the framerate reaches 50fps, the text should display "19.66Mbps @ 1280x1024 50fps"
    * When the framerate is decreased bewlow 50fps, the displayed framerate should scale down linearly with fps.
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
    "Mark End" buttons, ensuring that this consists of at least 4000 frames., then tap the Settings button to
    open the save settings window.
2. Insert a fast media device and select the H.264 encoding format, at 0.7 bits per pixel and 60fps.
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

---

Storage Media Tests
-------------------
TODO: Placeholder for mode advanced storage tests
* eSATA
* ext3 and ext4 filesystems
* file sizes in excess of 4GB.
* check for free space
* check for mounted filesystems.
How to add a new white balance preset
=====================
These steps allow one to add a new white balance preset to the white balance dialog.

1. I recommended using this branch to automatically take multiple white balances in a row and displays the average of them https://github.com/skronstein/chronos-cam-app/tree/white-balance-dialog-multiple
2. Compile and run the camapp on that branch. Open the white balance dialog, hit "Set Custom White Balance (Multiple)" and hit Yes.
3. Wait until the label at the top of the dialog changes its text to RGB values.  While waiting for the camera to take white balance multiple times and apply the average of them, keep the center of the image on a white card in the lighting for which you wish to add a preset for. The image may change color slightly many times while this is happening. At the end of this, the camapp will apply the average of all these white balances taken and, at the top of the white balance window, display the values you can use for a preset in the lighting conditions in which they were made. Write these values down somewhere.
4. In git, checkout the branch you want to work on(ie. master, not the white-balance-dialog-multiple branch). In the constructor of whitebalancedialog.cpp, Find the location where addPreset() is used several times, and insert a line there for the preset you want to add.
5. Increment the first index of "double sceneWhiteBalPresets[7][3];" in whitebalancedialog.h so that that number equal to the number of presets, including Custom and the newly added one.
6. Compile and test.

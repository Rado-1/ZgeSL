# Usage CML in ZGameEditor project #

  1. Open an ZGE project.
  1. Include `ZExternalLibrary` components for ZgeSL, MCI and CML to the `OnLoaded` section of `ZApplication`. All can be copied from the demos\player.zgeproj project.
  1. Use the `musicInit` function to initialize the library. Usually it is placed to the `OnLoaded` section.
  1. Use the `musicExit` function to release allocated resources. Usually it is placed to the `OnClose` section.
  1. Play the music file or URI by `musicPlayFile` function, stop playing by `musicStop)` function.
  1. Other functions can be used arbitrary to pause and resume playing (`musicPause`, `musicResume`), set volume (`musicSetVolume`), set panning (`musicSetPanning`), get duration (`musicGetDuration`), get playing status (`musicGetStatus`), get playing position (`musicGetPosition`), or set playing position (`musicSetPosition`).

For examples of usage ZgeSL in ZGE projects, download the demo application from [here](http://googledrive.com/host/0BxwfQ8la88ouM0NnQVZ3dHBMT2c/).

# Opening files on Android #

Use the following file names in the `musicPlayFile` call:

  * if the `<file>` is included in the application's _assets_ directory, use "`<file>`", or
  * if the `<file>` is placed in a `<directory>` on SDCARD, use "`file://sdcard/<directory>/<file>`", or
  * if the file is opened from `<uri>` Internet address, use "`<uri>`".

# Compiling Windows application #

Just use one of the Project Windows building menu items.

# Compiling Android application #

  1. Compile your Android application, e.g., use _Project / Android: Build APK (debug)_ menu item.
  1. Place _libZgeSL.so_ file to the _libs/armeabi_ folder. If the destination device supports the armv7a instruction set, it is recommended to use the library from _armeabi-v7a_ folder. If not and you want to support wider range of devices, use the library from _armeabi_ folder. They both work fine, but the version for armv7a achieves better performance.
  1. If you want to bundle mp3 file(s) to the built application, place them to the _assets_ directory.
  1. Compile the APK again as in step 1.
  1. Deploy the APK file to your Android device and install it in a standard way.

_Note 1: The 1st and 2nd steps are required only when the application is built the first time. After that, a single compilation produces correct APK._

_Note 2: Use the step 2 also in the case when updating to a newer version of the ZgeSL library._
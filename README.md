# Master-s-Thesis

### Running demo.exe
1) Download and install the [ASI Cameras ZWO Windows driver](https://www.zwoastro.com/downloads/windows) ([direct download](https://dl.zwoastro.com/software?app=AsiCameraDriver&region=Overseas))
2) Download and unzip the [ASI Camera SDK](https://www.zwoastro.com/downloads/developers)
3) add these paths to system PATH
	- SDK\lib\x86 				#(ASICamera2.dll)
	- SDK\demo\opencv2\lib 		#(opencv_imgproc247.lib)
	- SDK\demo\opencv2\bin		#(opencv_imgproc247.dll)
	or, alternatively, place the DDLs to the directory where you want to run the demos
4) run demo.exe


### Build the demo2.exe
1) download and install [Visual Studio 19 Community](https://visualstudio.microsoft.com/vs/older-downloads/) and add the C++ Development components
2) Visual Studio Installer > Modify > Individual Components:  
	**make sure the version of components you choose matches the version in the Project Properties (e.g. v142)**
	- MSVC v142 - VS 2019 C++ x64/86 build tools (latest)
	- C++ MFC for latest v142 build tools (x86 & x64)
	- C++ ATL for latest v142 build tools (x86 & x64)
3) Download and unzip the [ASI Camera SDK](https://www.zwoastro.com/downloads/developers)
4) Open demo2.sln

note: if it says some files exist from demo.sln and asks to replace them click "No"
5) Project Properties > Linker > Input > Additional Dependancies > Edit (little arrow on the right)
modify first line to: ../../lib/x86/ASICamera2.lib
6) Build the project (Right click on the project in the Solution Explorer on the right of the screen, or use the "Local Windows Debugger" button on the top)
7) demo2.exe is created in /Debug
note: demo2.exe in ../ is a prebuilt app, so it has nothing to do with new builds


notes:
 - if you accidentally add "SDK\lib\x64" to the path and run demo.exe, you will get: "The application was unable to start correctly (0xc000007b)"
 - the demo source code appears to be written for VS2010 SP1, so a migration to 2019 is required and expected
 - the original ReadMe.txt notes file has an encoding problem so the characters don't display properly. To fix, open the file in Notepad++ or rename it to ReadMe.html and open using a browser



# SPS 1 and 2

Once you configure your environment of choice, we can move on to building and running the SPS apps.
All the examples require additional codecs, Gstreamer libraries, Boost and the Tbb library, so we'll demonstrate installing those

## Windows, using VS19


1) Install vcpkg by following the [official instructions](https://vcpkg.io/en/getting-started.html)
2) Install the required libraries using:
```
vcpkg install boost #note: this took 28 minutes on the author's machine
vcpkg install opencv #note: this took 10 minutes on the author's machine
vcpkg install tbb
vcpkg install ffmpeg[core,avcodec,avdevice,avfilter,avformat,ffmpeg,ffplay,ffprobe,amf,opencl,x264,x265,swscale,nonfree] (Total install time: 15 min)
```
3) Run VS19 and create a new blank project
4) vcpkg installs x86 dependancies, so we need to choose x86 as the target in VS19. This can be done using a drop-down menu at the top, next to the "Local Windows Debugger" button
5) Copy and paste the code of the desired version into the new project
6) Build the project

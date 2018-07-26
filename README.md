# DWM StatusBar

![bar_left](images/bar_left.png "Left side of bottom bar")
![bar_right](images/bar_right.png "Right side of bottom bar")

A status bar for dwm.

Currently, it displays this system information:
* Network usage (download and upload)
* Disk usage
* Memory usage
* CPU usage (total and load)
* CPU temp
* Fan speed
* Brightness (screen and keyboard)
* System volume
* Battery usage/status
* WiFi status

... and this user information:
* TODO list updates
* Weather (current and forecasted)
* Backup logging


# Table of Contents #
* [Prerequisites](#prerequisites)
* [Installation](#installation)
* [Usage](#usage)
* [Getting Started](#getting-started)
	* [TODO file](#todo-file)
	* [Log status](#log-status)
	* [Weather](#weather)
	* [Backup status](#backup-status)
* [Macros](#macros)
* [Testing](#testing)
	* [Valgrind](#valgrind)
* [Contributing](#contributing)
* [Author](#author)
* [License](#license)
* [Acknowledgments](#acknowledgments)


## Prerequisites ##
DWM StatusBar works best with Andrew Milkovich's [dualstatus patch](https://dwm.suckless.org/patches/dualstatus/), which allows for displaying more information. If you choose not to use this patch in your setup, you will need to remove the semicolon separator that divides `top_bar` and `bottom_bar` in `format_string(Display *dpy, Window root)`.

All output is colored with the [colored status text](https://dwm.suckless.org/patches/statuscolors/) patch. If you do not want color or use a different color patch, you will need to update the source code for DWM StatusBar by modifying/removing the `COLOR*` macros in the print statements **AND** the `ERR` macro.

The weather JSONs from OpenWeatherMap.org are retrieved with [cURL](https://github.com/curl/curl), specifically libcurl. You can find installation instructions on their GitHub page or [website](https://curl.haxx.se/libcurl/). Yes, the URL looks sketchy, but it's legit.

Those JSONs are then parsed with Dave Gamble's lightweight [cJSON](https://github.com/DaveGamble/cJSON). You will need to install the headers and libraries to /usr/include/cJSON and /usr/lib (respectively) for the default installation or tell the compiler where to look for them in the Makefile. You can find more detailed instructions on the GitHub page.

Other standard linux libraries used that you should have by default:
* asound
* procps
* netlink
* libnl
* math (m)


## Installation ##
To download through GitHub:
* `git clone https://github.com/snhilde/dwm-statusbar`

Otherwise:
* download the zip
* open the archive with `unzip path/to/file.zip`

You will most likely have to work through the [Getting Started](#getting-started) and [Macros](#macros) sections for the compilation to go smoothly.

Just run `make` to build the executable. There is no install function in the Makefile. It is my belief that you will put the executable wherever you want to and are legally allowed. I keep it in my home folder's bin, so my personal Makefile includes this line:
```
all:
	@cp -f dwm-statusbar /home/user/bin/
```

## Usage ##
If you run the program directly, keep in mind that everything runs inside of a loop, making the program non-terminating (except for certain errors).

To run DWM StatusBar from the command line, type this:
```
/path/to/dwm-statusbar
```
To run it without specifying the path, you will need to place the bin in a folder in your terminal's path. You can add a directory to the path variable with a line in your shell's start-up script like this:
```
export PATH=/path/to/statusbar/directory/:$PATH
```
To start the program when X11 starts up (and you know you want to), add this line to your .xinitrc before the call for dwm:
```
/path/to/dwm-statusbar 2> /home/user/.logs/dwm-statusbar.log &
```
This will start the program, log the errors to dwm-statusbar.log, and not block the queue during startup.


## Getting Started ##
Because DWM StatusBar is designed to parse information thoroughly and with as little overhead as possible, there are some things that you will need to set up on your end to get the program running correctly. If you don't want to set up everything as instructed below, you should modify the source code to reflect your preferences.

For example, if you don't want to display the status of your backups, you should comment out **BOTH** function calls for `get_log_status()` in `loop()` and `init()` and remove the mention of `log_status_string` in `format_string()` for good measure. This is the simplest way to remove functionality. To change functionality, you will need to get your hands dirty and edit the source code directly.

Let's roll up our sleeves.

#### TODO file ####
I keep an ongoing TODO file for projects and languages and life and everything else that needs to be checked off. It is formatted like this:
```
Project Heading
	Current task
		subtask1
		subtask2
	Next task
	
Another Project
	Current task
```
... and so on. DWM StatusBar parses this file like so:
1. If there is nothing in the file, then congratulations, you have your life together. Nothing is printed.
2. Otherwise, the first project's heading is always printed.
3. If there is a task, it is concatenated and printed with a separating "->".
4. If there is a subtask, it is also concatenated and printed with a separating "->".
5. If there are no tasks but there is another project, it is printed with a separating "|".
6. No more than three lines are ever concatenated and printed (max one task and one subtask).

Are we having fun yet?

#### Log status ####
I keep my error logs for dwm and DWM StatusBar in a `.logs` directory in my home folder. This part of the program checks if there are errors in either log file and displays a message to check the log(s) if there are. To quiet the message, run `echo > /path/to/logfile` to reset the log.

Now we're warming up

#### Weather (current and forecast) ####
I might get rid of this section soon. I made it more for a learning opportunity than to get much-needed value. Let's be real; we all use our phones to check the weather. This function gets weather data for your location and outputs the current temp and highs/lows for the rest of the day and the next three days. If there is a forecast for rain, the day gets highlighted. There is no functionality yet for other major weather patterns. Sorry, Russia.

To get data for your location, you will need to find your city's ID [here](https://openweathermap.org/appid). While you're there, also study up on getting your own free API key. You're not using mine. Your city ID will go in the header for the macro `LOCATION`, and the API key will go in the macro `KEY`.

This is where it gets real

#### Backup status ####
This one is a whole other beast. I won't feel bad at all if you decide to leave it out (by removing the function calls in `init()` and `loop()`). We're all friends here. 

If you do leave it in, you will need to customize the function heavily. My script creates a backup, encrypts it, and uploads it to a server, and prints each of those steps into a log file. The function parses that file for those strings or error codes like this:
1. If the contents of the file are digits, then it parses the number for an error code in `parse_error_code(int code)`. That code is converted into an error message and printed to the bar.
2. If the contents of the file are digits but not an error code, then it must be the time in seconds since the epoch when the backup was completed. This number is calculated against the current time to get the time elapsed and printed accordingly.
3. If the contents of the file are not digits, then the string is printed straight to the bar.

To customize this function, you will want to update the error codes of your backup script, chiefly.

**Nice work**


## Macros ##

Because computers differ, there are a few things that you must configure before running DWM StatusBar. All values and options are contained in the header file. Let's walk through them:
- - - -
```
#define COLOR_NORMAL			color1
#define COLOR_ACTIVE			color2
#define COLOR1				color3
#define COLOR2				color4
#define COLOR_WARNING			color5
#define COLOR_ERROR			color6
#define COLOR_HEADING			COLOR_ACTIVE

const char color1 = '';
const char color2 = '';
const char color3 = '';
const char color4 = '';
const char color5 = '';
const char color6 = '';
```
These colors correspond to the colors that you defined in your config.h for DWM. Make any changes you think are pretty.
- - - -
```
#define TODO_MAX_LEN			100
```
Maximum number of characters to display in the TODO string.
- - - -
```
#define WIFI_MAX_LEN			24
```
Maximum number of characters to display in the WiFi SSID string.
- - - -
```
#define WIFI_INTERFACE			"wlp4s0"
```
The name of your wireless device. You can find it by running `iw dev` or `ip link`.
- - - -
```
#define DISPLAY_KBD			true
```
True if you want to also display the keyboard backlight, false if not.
- - - -
**The next items are locations of files that you should localize for your system:**
- - - -
```
#define TODO_FILE			"/home/user/.TODO"
```
Your personal TODO file
- - - -
```
#define STATUSBAR_LOG_FILE		"/home/user/.logs/dwm-statusbar.log"
```
The log file for DWM StatusBar's errors
- - - -
```
#define BACKUP_STATUS_FILE		"/home/user/.backup/.sb"
```
The status output file for your backups
- - - -
```
#define LOCATION				"0000000"
```
The numerical city id as described in [Getting Started](#getting-started) (#3)
- - - -
```
#define KEY				"reallylongstringthatisreallylong"
```
Your API key from OpenWeatherMap as described in [Getting Started](#getting-started) (#3)
- - - -
```
#define DWM_CONFIG_FILE			"/home/user/.dwm/config.h"
```
Your personal config file for dwm
- - - -
```
#define CPU_TEMP_DIR			"/sys/class/hwmon/hwmon1/"
```
The directory for accessing CPU temperature data. It will include files like "temp1_input" and others. DWM StatusBar automatically finds and parses all the temperature files in the directory.
- - - -
```
#define FAN_SPEED_FILE			"/sys/class/hwmon/hwmon2/device/fan1_input"
```
The file that corresponds to the fan for the CPU
- - - -
```
#define SCREEN_BRIGHTNESS_FILE		"/sys/class/backlight/nvidia_backlight/brightness"
```
This will change depending on the video driver you use
- - - -
```
#define KBD_BRIGHTNESS_FILE		"/sys/class/leds/smc::kbd_backlight/brightness"
```
If you don't have a keyboard backlight, make sure you switch the DISPLAY_KBD macro to "false"


## Testing ##

Testing individual functions is very easy, especially in dwm.

To limit the program to one or more specific functions, just comment out the function call in **BOTH** `init()` and `loop()`. You must do both, or you will be very confused. Please share the results of your tests with the community to better the project.

#### Valgrind ####
This is the reference command for running valgrind:
```
valgrind -v --leak-check=yes --undef-value-errors=no --log-file=valgrind.log ./dwm-statusbar
```
After fixing various memory leaks (thanks, valgrind), I have been unable to completely seal the program due to the X11 library, specifically from `XCreateFontSet`, which is throwing memory leak problems in two different areas:
1. An unpaired `malloc` for 18 bytes in 1 block
2. An unpaired `realloc` for 192 bytes in 1 block following this trail:
XCreateFontSet (in /usr/lib/libX11.so.6.3.0)
	-> XOpenOM (in /usr/lib/libX11.so.6.3.0)
		-> _XOpenLC (in /usr/lib/libX11.so.6.3.0)
			-> _XlcDefaultLoader (in /usr/lib/libX11.so.6.3.0)
				-> _XlcCreateLC (in /usr/lib/libX11.so.6.3.0)
				
I have been unable to figure out how to fix this leak so far. Because `XCreateFontSet` gets called just one time (in the initialization), there is a total leak of only 200 bytes during the entire life of the program. That's not bad, so I'm leaving it as is for now until a rainy day.

## Contributing ##

Send a pull request or a message. Additional functionality is welcome, as are suggestions to make the program leaner, faster, and better performing. #LebronThisCode


## Author ##

Hilde N


## License ##

This project is licensed under the MIT License. Do whatever you want with it. See the [LICENSE.md](LICENSE.md) file for details


## Acknowledgments ##

* Jean Tourrilhes and the developers of the Wireless Tools suite for Linux
* Stephen Hemminger and the team behind iproute2
* Dave Gamble for cJSON
* Sebastien Godard and the valiant effort of everyone who worked on sysstat
* J. C. Warner and the folks of top (J because it's either Jim or James, help me out here)
* Jaroslav Kysela and the team that made and maintains ALSA
* Everyone behind acpi
* The fine folks at OpenWeatherMap
* David Brailsford
* [Paul Whipp](paulwhippconsulting.com)
* What is this, an award show?

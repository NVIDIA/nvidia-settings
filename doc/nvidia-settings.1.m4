changequote([[[, ]]])dnl
define(__OPTIONS__, [[[include([[[options.1.inc]]])dnl]]])dnl
dnl Solaris man chokes on three-letter macros.
ifelse(__BUILD_OS__,SunOS,[[[define(__URL__,UR)]]],[[[define(__URL__,URL)]]])dnl
.\" Copyright (C) 2010 NVIDIA Corporation.
__HEADER__
.\" Define the .__URL__ macro and then override it with the www.tmac package if it
.\" exists.
.de __URL__
\\$2 \(la \\$1 \(ra\\$3
..
.if \n[.g] .mso www.tmac
.TH nvidia\-settings 1 "2018-03-20" "nvidia\-settings __VERSION__"
.SH NAME
nvidia\-settings \- configure the NVIDIA graphics driver
.SH SYNOPSIS
.BI "nvidia\-settings [" "options" "]"
.br
.BI "nvidia\-settings [" "options" "] \-\-no\-config"
.br
.BI "nvidia\-settings [" "options" "] \-\-load\-config\-only"
.br
.BI "nvidia\-settings [" "options" "] {\-\-query=" attr " | \-\-assign=" attr = value "} ..."
.br
.BI "nvidia\-settings [" "options" "] \-\-glxinfo"
.PP
Options:
.BI "[\-vh] [\-\-config=" configfile "] [\-c " ctrl-display "]"
.br
.I "         \fB[\-\-verbose=\fP{\fInone \fP|\fI errors \fP|\fI deprecations \fP|\fI warnings \fP|\fI all\fP}\fB]"
.br
.I "         \fB[\-\-describe=\fP{\fIall \fP|\fI list \fP|\fI attribute_name\fP}\fB]"
.PP
.I attr
has the form:
.ti +5
.IB DISPLAY / attribute_name [ display_devices ]
.SH DESCRIPTION
The
.B nvidia\-settings
utility is a tool for configuring the NVIDIA graphics driver.
It operates by communicating with the NVIDIA X driver, querying and updating state as appropriate.
This communication is done via the NV-CONTROL, GLX, XVideo, and RandR X extensions.
.PP
Values such as brightness and gamma, XVideo attributes, temperature, and OpenGL settings can be queried and configured via
.B nvidia\-settings.
.PP
When
.B nvidia\-settings
starts, it reads the current settings from its configuration file and sends those settings to the X server.
Then, it displays a graphical user interface (GUI) for configuring the current settings.
When
.B nvidia\-settings
exits, it queries the current settings from the X server and saves them to the configuration file.
dnl Call gen-manpage-opts to generate this section.
__OPTIONS__
.SH "USER GUIDE"
.SS Contents
1.	Layout of the nvidia\-settings GUI
.br
2.	How OpenGL Interacts with nvidia\-settings
.br
3.	Loading Settings Automatically
.br
4.	Command Line Interface
.br
5.	X Display Names in the Config File
.br
6.	Connecting to Remote X Servers
.br
7.	Licensing
.br
8.	TODO
.br
.SS 1. Layout of the nvidia\-settings GUI
The
.B nvidia\-settings
GUI is organized with a list of different categories on the left side.
Only one entry in the list can be selected at once, and the selected category controls which "page" is displayed on the right side of the
.B nvidia\-settings
GUI.
.PP
The category list is organized in a tree: each X screen contains the relevant subcategories beneath it.
Similarly, the Display Devices category for a screen contains all the enabled display devices beneath it.
Besides each X screen, the other top level category is "nvidia\-settings Configuration", which configures behavior of the
.B nvidia\-settings
application itself.
.PP
Along the bottom of the
.B nvidia\-settings
GUI, from left to right, is:
.TP
1)
a status bar which indicates the most recently altered option;
.TP
2)
a Help button that toggles the display of a help window which provides a detailed explanation of the available options in the current page; and
.TP
3)
a Quit button to exit
.B nvidia\-settings.
.PP
Most options throughout
.B nvidia\-settings
are applied immediately.
Notable exceptions are OpenGL options which are only read by OpenGL when an OpenGL application starts.
.PP
Details about the options on each page of
.B nvidia\-settings
are available in the help window.
.SS 2. How OpenGL Interacts with nvidia\-settings
.PP
When an OpenGL application starts, it downloads the current values from the X driver, and then reads the environment (see
.I APPENDIX E: OPENGL ENVIRONMENT VARIABLE SETTINGS
in the README).
Settings from the X server override OpenGL's default values, and settings from the environment override values from the X server.
.PP
For example, by default OpenGL uses the FSAA setting requested by the application (normally, applications do not request any FSAA).
An FSAA setting specified in
.B nvidia\-settings
would override the OpenGL application's request.
Similarly, the
.B __GL_FSAA_MODE
environment variable will override the application's FSAA setting, as well as any FSAA setting specified in
.B nvidia\-settings.
.PP
Note that an OpenGL application only retrieves settings from the X server when
it starts, so if you make a change to an OpenGL value in
.B nvidia\-settings,
it will only apply to OpenGL applications which are started after that point in time.
.SS 3. Loading Settings Automatically
The NVIDIA X driver does not preserve values set with
.B nvidia\-settings
between runs of the X server (or even between logging in and logging out of X, with
.BR xdm (1),
.B gdm,
or
.B kdm
).
This is intentional, because different users may have different preferences, thus these settings are stored on a per-user basis in a configuration file stored in the user's home directory.
.PP
The configuration file is named
.IR ~/.nvidia\-settings\-rc .
You can specify a different configuration file name with the
.B \-\-config
command line option.
.PP
After you have run
.B nvidia\-settings
once and have generated a configuration file, you can then run:
.sp
.ti +5
nvidia\-settings \-\-load\-config\-only
.sp
at any time in the future to upload these settings to the X server again.
For example, you might place the above command in your
.I ~/.xinitrc
file so that your settings are applied automatically when you log in to X.
.PP
Your
.I .xinitrc
file, which controls what X applications should be started when you log into X (or startx), might look something like this:
.nf

     nvidia\-settings \-\-load\-config\-only &
     xterm &
     evilwm

.fi
or:
.nf

     nvidia\-settings \-\-load\-config\-only &
     gnome\-session

.fi
If you do not already have an
.I ~/.xinitrc
file, then chances are that
.BR xinit (1)
is using a system-wide xinitrc file.
This system wide file is typically here:
.nf

     /etc/X11/xinit/xinitrc

.fi
To use it, but also have
.B nvidia\-settings
upload your settings, you could create an
.I ~/.xinitrc
with the contents:
.nf

     nvidia\-settings \-\-load\-config\-only &
     . /etc/X11/xinit/xinitrc

.fi
System administrators may choose to place the
.B nvidia\-settings
load command directly in the system xinitrc script.
.PP
Please see the
.BR xinit (1)
man page for further details of configuring your
.I ~/.xinitrc
file.
.SS 4. Command Line Interface
.B nvidia\-settings
has a rich command line interface: all attributes that can be manipulated with the GUI can also be queried and set from the command line.
The command line syntax for querying and assigning attributes matches that of the
.I .nvidia\-settings\-rc
configuration file.
.PP
The
.B \-\-query
option can be used to query the current value of attributes.
This will also report the valid values for the attribute.
You can run
.B nvidia\-settings \-\-query all
for a complete list of available attributes, what the current value is, what values are valid for the attribute, and through which target types (e.g., X screens, GPUs) the attributes can be addressed.
Additionally, individual attributes may be specified like this:
.nf

        nvidia\-settings \-\-query Overlay

.fi
An attribute name may be prepended with an X Display name and a forward slash to indicate a different X Display; e.g.:
.nf

        nvidia\-settings \-\-query localhost:0.0/Overlay

.fi
An attribute name may also just be prepended with the screen number and a forward slash:
.nf

        nvidia\-settings \-\-query 0/Overlay

.fi
in which case the default X Display will be used, but you can indicate to which X screen to direct the query (if your X server has multiple X screens).
If no X screen is specified, then the attribute value will be queried for all valid targets of the attribute (eg GPUs, Displays X screens, etc).
.PP
Attributes can be addressed through "target types".
A target type indicates the object that is queried when you query an attribute.
The default target type is an X screen, but other possible target types are GPUs, Frame Lock devices, Visual Computing Systems, fans, thermal sensors, 3D Vision Pro Transceivers and display devices.
.PP
Target types give you different granularities with which to perform queries and assignments.
Since X screens can span multiple GPUs (in the case of Xinerama, or SLI), and multiple X screens can exist on the same GPU, it is sometimes useful to address attributes by GPU rather than X screen.
.PP
A target specification is contained within brackets and may consist of a target type name, a colon, and the target id.
The target type name can be one of
.B screen,
.B gpu,
.B framelock,
.B fan,
.B thermalsensor,
.B svp,
or
.B dpy;
the target id is the index into the list of targets (for that target type).
Target specifications can be used wherever an X screen is used in query and assignment commands; the target specification can be used either by itself on the left side of the forward slash, or as part of an X Display name.
.PP
For example, the following queries address X screen 0 on the localhost:
.nf

        nvidia\-settings \-\-query 0/VideoRam
        nvidia\-settings \-\-query localhost:0.0/VideoRam
        nvidia\-settings \-\-query [screen:0]/VideoRam
        nvidia\-settings \-\-query localhost:0[screen:0]/VideoRam

.fi
To address GPU 0 instead, you can use either of:
.nf

        nvidia\-settings \-\-query [gpu:0]/VideoRam
        nvidia\-settings \-\-query localhost:0[gpu:0]/VideoRam

.fi
Note that if a target specification is present, it will override any X screen specified in the display name as the target to process.
For example, the following query would address GPU 0, and not X screen 1:
.nf

	nvidia\-settings \-\-query localhost:0.1[gpu:0]/VideoRam

.fi
.PP
A target name may be used instead of a target id, in which case all targets with matching names are processed.
.PP
For example, querying the DigitalVibrance of display device DVI-I-1 may be done like so:
.nf

	nvidia\-settings \-\-query [dpy:DVI\-I\-1]/DigitalVibrance

.fi
When a target name is specified, the target type name may be omitted, though this should be used with caution since the name will be matched across all target types.
The above example could be written as:
.nf

	nvidia\-settings \-\-query [DVI\-I\-1]/DigitalVibrance

.fi
The target name may also simply be a target type name, in which case all targets of that type will be queried.
.PP
For exmple, querying the BusRate of all GPUs may be done like so:
.nf

	nvidia\-settings \-\-query [gpu]/BusRate

.fi
.PP
The target specification may also include a target qualifier.
This is useful to limit processing to a subset of targets, based on an existing relationship(s) to other targets.
The target qualifier is specified by prepending a target type name, a colon, the target id, and a period to the existing specification.
Only one qualitfer may be specified.
.PP
For example, querying the RefreshRate of all DFP devices on GPU 1 may be done like so:
.nf

	nvidia\-settings \-\-query [GPU:1.DPY:DFP]/RefreshRate

.fi
Likewise, a simple target name (or target type name) may be used as the qualifier.
For example, to query the BusType of all GPUs that have DFPs can be done like so:
.nf

	nvidia\-settings \-\-query [DFP.GPU]/BusType

.fi
.PP
See the output of
.nf

        nvidia\-settings \-\-query all

.fi
for what targets types can be used with each attribute.
See the output of
.nf

        nvidia\-settings \-\-query screens \-\-query gpus \-\-query framelocks \-\-query fans \-\-query thermalsensors \-\-query svps \-\-query dpys

.fi
for lists of targets for each target type.
.PP
To enable support for the "GPUGraphicsClockOffset" and "GPUMemoryTransferRateOffset" attributes, ensure that the "Coolbits" X configuration option includes the value "8" in the bitmask.
For more details, refer to the documentation of the "Coolbits" option in the NVIDIA driver README.
Query the "GPUPerfModes" string attribute to see a list of the available performance modes:
.PP
.nf

	nvidia\-settings \-\-query GPUPerfModes 

.fi
.PP
Each performance mode is presented as a comma-separated list of "token=value" pairs.
Each set of performance mode tokens is separated by a ";".
The "perf" token indicates the performance level.
The "*editable" tokens indicate which domains within the performance level can have an offset applied. 
The "GPUGraphicsClockOffset" and "GPUMemoryTransferRateOffset" attributes map respectively to the "nvclock" and "memtransferrate" tokens of performance levels in the "GPUPerfModes" string.
.PP
Note that the clock manipulation attributes "GPUGraphicsClockOffset" and "GPUMemoryTransferRateOffset" apply to the offsets of specific performance levels.
The performance level is specified in square brackets after the attribute name.
For example, to query the "GPUGraphicsClockOffset" for performance level 2:
.PP
.nf

	nvidia\-settings \-\-query GPUGraphicsClockOffset[2]

.fi
The
.B \-\-assign
option can be used to assign a new value to an attribute.
The valid values for an attribute are reported when the attribute is queried.
The syntax for
.B \-\-assign
is the same as
.B \-\-query,
with the additional requirement that assignments also have an equal sign and the new value.
For example:
.nf

        nvidia\-settings \-\-assign FSAA=2
        nvidia\-settings \-\-assign [CRT\-1]/DigitalVibrance=9
        nvidia\-settings \-\-assign [gpu:0]/DigitalVibrance=0
        nvidia\-settings \-\-assign [gpu:0]/GPUGraphicsClockOffset[2]=10
.fi
.PP
Multiple queries and assignments may be specified on the command line for a single invocation of
.B nvidia\-settings.
Assignments are processed in the order they are entered on the command line.
If multiple assignments are made to the same attribute or to multiple attributes with dependencies, then the later assignments will have priority.
.PP
If either the
.B \-\-query
or
.B \-\-assign
options are passed to
.B nvidia\-settings,
the GUI will not be presented, and
.B nvidia\-settings
will exit after processing the assignments and/or queries.
In this case, settings contained within the
.I ~/.nvidia\-settings\-rc
configuration file will not be automatically uploaded to the X server, nor will the
.I ~/.nvidia\-settings\-rc
configuration file be automatically updated to reflect attribute assignments made via the
.B \-\-assign
option.
.SS 5. X Display Names in the Config File
In the Command Line Interface section above, it was noted that you can
specify an attribute without any X Display qualifiers, with only an X
screen qualifier, or with a full X Display name.
For example:
.nf

        nvidia\-settings \-\-query FSAA
        nvidia\-settings \-\-query 0/FSAA
        nvidia\-settings \-\-query stravinsky.nvidia.com:0/FSAA

.fi
In the first two cases, the default X Display will be used, in the second case, the screen from the default X Display can be overridden, and in the third case, the entire default X Display can be overridden.
.PP
The same possibilities are available in the
.I ~/.nvidia\-settings\-rc
configuration file.
.PP
For example, in a computer lab environment, you might log into any of multiple
workstations, and your home directory is NFS mounted to each workstation.
In such a situation, you might want your
.I ~/.nvidia\-settings\-rc
file to be applicable to all the workstations.
Therefore, you would not want your config file to qualify each attribute with an X Display Name.
Leave the "Include X Display Names in the Config File" option unchecked on the
.B nvidia\-settings
Configuration page (this is the default).
.PP
There may be cases when you do want attributes in the config file to be qualified with the X Display name.
If you know what you are doing and want config file attributes to be qualified with an X Display, check the "Include X Display Names in the Config File" option on the
.B nvidia\-settings
Configuration page.
.PP
In the typical home user environment where your home directory is local to one computer and you are only configuring one X Display, then it does not matter whether each attribute setting is qualified with an X Display Name.
.SS 6. Connecting to Remote X Servers
.B nvidia\-settings
is an X client, but uses two separate X connections: one to display the GUI, and another to communicate the NV-CONTROL requests.
These two X connections do not need to be to the same X server.
For example, you might run
.B nvidia\-settings
on the computer stravinsky.nvidia.com, export the display to the computer bartok.nvidia.com, but be configuring the X server on the computer schoenberg.nvidia.com:
.nf

        nvidia\-settings \-\-display=bartok.nvidia.com:0 \\
            \-\-ctrl\-display=schoenberg.nvidia.com:0

.fi
If
.B \-\-ctrl\-display
is not specified, then the X Display to control is what
.B \-\-display
indicates.
If
.B \-\-display
is also not specified, then the
.I $DISPLAY
environment variable is used.
.PP
Note, however, that you will need to have X permissions configured such that you can establish an X connection from the computer on which you are running
.B nvidia\-settings
(stravinsky.nvidia.com) to the computer where you are displaying the GUI (bartok.nvidia.com) and the computer whose X Display you are configuring (schoenberg.nvidia.com).
.PP
The simplest, most common, and least secure mechanism to do this is to use 'xhost' to allow access from the computer on which you are running
.B nvidia\-settings.
.nf

        (issued from bartok.nvidia.com)
        xhost +stravinsky.nvidia.com

        (issued from schoenberg.nvidia.com)
        xhost +stravinsky.nvidia.com

.fi
This will allow all X clients run on stravinsky.nvidia.com to connect and display on bartok.nvidia.com's X server and configure schoenberg.nvidia.com's X server.
.PP
Please see the
.BR xauth (1)
and
.BR xhost (1)
man pages, or refer to your system documentation on remote X applications and security.
You might also Google for terms such as "remote X security" or "remote X Windows", and see documents such as the Remote X Apps mini-HOWTO:
.sp
.ti +5
.__URL__ http://www.tldp.org/HOWTO/Remote-X-Apps.html
.sp
Please also note that the remote X server to be controlled must be using the NVIDIA X driver.
.SS 7. Licensing
The source code to
.B nvidia\-settings
is released as GPL.
The most recent official version of the source code is available here:
.sp
.ti +5
.__URL__ https://download.nvidia.com/XFree86/nvidia-settings/
.sp
Note that
.B nvidia\-settings
is simply an NV-CONTROL client.
It uses the NV-CONTROL X extension to communicate with the NVIDIA X server to query current settings and make changes to settings.
.PP
You can make additions directly to
.B nvidia\-settings,
or write your own NV-CONTROL client, using
.B nvidia\-settings
as an example.
.PP
Documentation on the NV-CONTROL extension and additional sample clients are available in the
.B nvidia\-settings
source tarball.
Patches can be submitted to linux\-bugs@nvidia.com.
.SS 8. TODO
There are many things still to be added to
.B nvidia\-settings,
some of which include:
.TP
-
different toolkits?
The GUI for
.B nvidia\-settings
is cleanly abstracted from the back-end of
.B nvidia\-settings
that parses the configuration file and command line, communicates with the X server, etc.
If someone were so inclined, a different front-end GUI could be implemented.
.TP
-
write a design document explaining how
.B nvidia\-settings
is designed; presumably this would make it easier for people to become familiar with the code base.
.PP
If there are other things you would like to see added (or better yet, would like to add yourself), please contact linux\-bugs@nvidia.com.
.SH FILES
.TP
.I ~/.nvidia\-settings\-rc
.SH EXAMPLES
.TP
.B nvidia\-settings
Starts the
.B nvidia\-settings
graphical interface.
.TP
.B nvidia\-settings \-\-load\-config\-only
Loads the settings stored in
.I ~/.nvidia\-settings\-rc
and exits.
.TP
.B nvidia\-settings \-\-rewrite\-config\-file
Writes the current X server configuration to
.I ~/.nvidia\-settings\-rc
file and exits.
.TP
.B nvidia\-settings \-\-query FSAA
Query the value of the full-screen antialiasing setting.
.TP
.B nvidia\-settings \-\-assign RedGamma=2.0 \-\-assign BlueGamma=2.0 \-\-assign GreenGamma=2.0
Set the gamma of the screen to 2.0.
.SH AUTHOR
Aaron Plattner
.br
NVIDIA Corporation
.SH "SEE ALSO"
.BR nvidia\-xconfig (1)ifelse(__BUILD_OS__,Linux,[[[,
.BR nvidia\-installer (1)]]])
.SH COPYRIGHT
Copyright \(co 2010 NVIDIA Corporation.

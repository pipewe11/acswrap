# acswrap
Assetto Corsa Server Wrapper

# Introduction
I'm running some Asetto Corsa servers on a cheap virtual server.
I once wanted to put download links and images in the server details pane of Content Manager. Available memory space was limited on a virtual server, so I made this wrapper. I think that the memory footprint of the program is less than 3MB.

![screenshot](https://user-images.githubusercontent.com/24981419/190940277-6042d96c-de56-48e4-ae65-65166b6d0d39.png)

# Preparing for the build

You need the following in order to build GCC:
* clang or gcc supports C++11
* boost/format
* boost/program_options
* boost/process
* boost/filesystem

Ubuntu Users
```
> apt install -y clang-12
> apt install -y libboost-dev
> apt install -y libboost-program-options-dev
> apt install -y libboost-filesystem-dev
```

# Downloading the Source Code
```
git clone https://github.com/pipewe11/acswrap.git
```
# The Build
```
cd acswrap
make
```
# Installation

Copy "acswrap" to anywhere you want to place the executable.

# Using the Wrapper
```
/path/to/acswrap -p 8091 -s ./acServer -c cfg/server_cfg.ini -e cfg/entry.ini -d desc.txt -n 5
```
Please note that "acServer" in the cloned working directory is a dummy program for testing purpose only, so you don't need to use it.

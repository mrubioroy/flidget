FLIDGET
=======

DESCRIPTION
-----------

flidget is a flexible widget system.

flidget creates wigets over your desktop with graphs (e.g.: cpu load) or command outputs (e.g.: system log). The data feeding thoses boxes comes from external commands. A few are provided with flidget: cpu load, cpu temperature, cpu frequency and net load. You can write your own easily. Just output to stdout the values you want to plot or print.

Colors, position and size of the widgets can be customize using a CSS file.

See the man page flidget(1) (or `man man/man1/flidget.1`) for details on customizing the widgets.

SCREENSHOT
----------

![screenshot](doc/screenshot.png?raw=true)

INSTALLATION ON DEBIAN-BASED SYSTEMS
------------------------------------

1. Download the latest deb release from the project:

   https://github.com/mrubioroy/flidget/releases

   Install the deb package as root (replace VERSION appropriately):
```
sudo dpkg -i flidget_VERSION.deb
```

2. Example files of flidget.conf and flidget.css can be found at /usr/share/doc/flidget/ Copy and ungzip those two files to ~/.config/flidget/ (the default configuration folder)

3. If using autostart, copy flidget.desktop.gz from /usr/share/doc/flidget/ to ~/.config/autostart/, ungzip, log out and log in. Otherwise, start with:
```
flidget
```

INSTALLATION ON OTHER SYSTEMS (BUILD)
-------------------------------------

1. Get the latest source code tarball from the project:

   https://github.com/mrubioroy/flidget/releases

   Untar, compile and install flidget at /usr/local/bin (replace VERSION appropriately):
```
tar -xf flidget-VERSION.tar.gz
cd flidget-VERSION
sudo make local_install
```

3. Example files of flidget.conf and flidget.css can be found at the doc folder. Copy them to ~/.config/flidget/ (the default configuration folder)

4. If using autostart, copy flidget.desktop from the extracted directory to ~/.config/autostart/, log out and log in. Otherwise, start with:
```
flidget
```

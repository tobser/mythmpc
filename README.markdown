# MythMpc

A simple mythtv plugin to remote control MPD (Music Player Daemon).

This plugin is of no use to you without MPD.

## Features
* Display current playlist
* play/pause/stop
* jump to next/previous track
* Volume up/down and mute

## Not Working
* loading/management of playlists
* changing the current play queue

## Installation
    apt-get install qt5-default ccache g++ libmpdclient-dev mythtv-frontend libmyth-dev
    git clone git://github.com/tobser/mythmpc.git
    cd mythmpc
    ./configure
    make
    sudo make install

## Setup
### Configuration
To configure MPD _host/port/password_: Start the plugin and press the menu button on your remote.

### Add a Button
Add a button to your library.xml menu (see the [Menu theme development
guide](http://www.mythtv.org/wiki/Menu_theme_development_guide)):

    <button>
        <type>MUSIC_PLAY</type>
        <text>MythMpc MPD Client</text>
        <description>Browse your MPD music library</description>
        <action>PLUGIN mythmpc</action>
        <depends>mythmpc</depends>
    </button>

### Or use Jumppoint 
Assing a key on your remote to start mythmpc: `Setup`->`Edit Keys`->`JumpPonts`->`MPC`

### autostart mythmpc plugin if mpd is playing music
To immediatly enter mythmpc if `mpd` is currently in playback you can use this shell script.
The script needs `mpc` (`apt-get install mpc`) to check the current playback state of `mpd`

    #!/usr/bin/env bash
    
    ##
    ## checks the current state of mpd, if in playback mythmpc plugin is
    ## laoded, else mythfrontend starts with live tv playback
    ##
    
    frontend="/usr/bin/mythfrontend"
    mpc="/usr/bin/mpc"
    mpd_state=`$mpc -h mpd_host_name -p 6600 |grep "\[.*\]" | sed 's/\[\(.*\)\].*/\1/'`
    
    if [ "X$mpd_state" == "Xplaying" ]; then
        ($frontend --runplugin "mpc")
    else
        ($frontend --jumppoint "Live TV")
    fi

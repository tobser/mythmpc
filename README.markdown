# MythMpc

A simple mythtv plugin to remote control MPD (Music Player Daemon).

This plugin is of no use to you without MPD.

## Features
* Display current playlist
* play/pause/stop
* jump to next/previous track
* Volume up/down and mute

## Installation
    apt-get install libmpdclient-dev
    git clone git://github.com/tobser/mythmpc.git
    cd mythmpc
    ./configure
    make
    sudo make install

## Setup

Add a button to your library.xml menu (see the [Menu theme development
guide](http://www.mythtv.org/wiki/Menu_theme_development_guide)):

    <button>
        <type>MUSIC_PLAY</type>
        <text>MythMpc MPD Client</text>
        <description>Browse your MPD music library</description>
        <action>PLUGIN mythmpc</action>
        <depends>mythmpc</depends>
    </button>

To configure MPD _host/port/password_: Start the plugin and press the menu button on your remote.

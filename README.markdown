# MythMpc

A simple mythtv plugin to remote control MPD (Music Player Daemon).

This plugin is of no use to you without MPD.

## Features
    * Display current playlist
    * play/pause/stop
    * jump to next/previous track
    * Volume up/down and mute

## Installation

    git clone git://github.com/tobser/mythmpc.git
    cd mythmpc
    ./configure
    make
    sudo make install

## Configuration

you may simply add a button to your menu (see the [Menu theme development
guide](http://www.mythtv.org/wiki/Menu_theme_development_guide))

    <button>
        <type>MUSIC_PLAY</type>
        <text>MythMpc MPD Client</text>
        <description>Browse your MPD music library</description>
        <action>PLUGIN mythmpc</action>
        <depends>mythmpc</depends>
    </button>

To setup MPD host/port/password: Start the plugin and  press the menu button on your remote.

# ACARSDEC

Acarsdec is an open source, realtime  ACARS demodulator and position decoder for Linux.

Aircraft Communication Addressing and Reporting System (or ACARS) is a digital datalink system for transmission of small messages between aircraft and ground stations via VHF radio.

## HOW DOES IT WORK ?

To receive ACARS you need at least an AM VHF air band receiver tuned to one of these frequencies :

- 131.725	Europe primary
- 131.525	European secondary
- 131.550	USA primary
- 130.025	USA secondary
- 131.450 Japan primary
(these are the most common, google is your friend for other frequencies)

Audio output from this receiver is send to the soundcard input of your PC under Linux.
Then, acarsdec will demodulate the signals sent by aircrafts and print the received messages on its standart output in airnav log text format.

## BUILDING IT
On a Linux system, you will need libsnd librairy, alsa audio system and gcc/make installed.
Then just type :
`make`

## USING IT
acarsdec could be called with the following options :
```
acarsdec [-LR][-s noport] -d alsapcmdevice | -f sndfile
 -f sndfile :           decode from file sndfile (ie: a .wav file)
 -d alsapcmdevice :     decode from soundcard input alsapcmdevice (ie: hw:0,0)
 [-LR] :                disable left or right channel decoding of stereo signal (save cpu)
 [-s noport ] :         "xastir" mode : act as an APRS local server, on port : noport (see below)
```

Input could be mono or stereo but with 48Khz sampling frequency.
If stereo, acarsdec will demod the 2 channels independantly (if no L ou R options specified)

Typical usage for realtime decoding is :
`acarsdec -d hw:0`

Be sure that correct record level is set for the used soundcard input.
For testing, you could try to record your receiver output at 48khz sampling frequency with any audio recording tool.
Save as wav file, then decode it by :
`acarsdec -f audiofile.wav`


## USING IT WITH XASTIR
acarsdec have a special output mode to use it with APRS position reporting plotting program : [xastir](www.xastir.org).
In this mode, acarsdec acts as a very basic local aprsd server.
ACARS messages, and in particular, position report messages are converted to APRS format, so you can plot aircraft positions on a map.

PS: position decoding is in experimental stage. Mail me if you find errors or lack of position reporting.

start acarsdec with the following option :
`acarsdec -d hw:0 -s 14000`

Then in xastir, choose : Interface->Interface Control->Add
Select : Internet Server, then Add
Set Host at 127.0.0.1, Port 14000, Don't allow transmitting, then Ok.
This will add an interface in the Interface Control dialog.

Then select this interface and press start.
To check that acarsdec send messages to xastir, select View->Incoming traffic
ACARS messages look like that in xastir :
F-XXYZ>ACARS:>Fid:AFXXXX Lbl:Q0

Lots of ACARS messages are messages without position report, so be patient before seeing aircraft plotted on the map.



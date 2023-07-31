# Mixijo

Lightweight audio mixer for ASIO devices.

![image](https://assets.kaixo.me/mixijo.png)

## How To Setup
The easiest way to set Mixijo up is to start it up. 
The console window should then print the available ASIO devices for you.
Here's an example of what it could output:
```
[Mixijo] 2022-10-02 07:48:25 [err] No audio device found with the name (My Audio Device)
[Mixijo] 2022-10-02 07:48:25 [log] available audio devices:
[Mixijo] 2022-10-02 07:48:25 [log]   FL Studio ASIO
[Mixijo] 2022-10-02 07:48:25 [log]   Focusrite USB ASIO
[Mixijo] 2022-10-02 07:48:25 [log]   Synchronous Audio Router
```
Then you open the `settings.json` file, and edit the `"audio"` field to the device you want to use:
```
{
    "audio": "Synchronous Audio Router",
    "samplerate": 44100,
    "buffersize": 256,
    ...
}
```

Once you've saved the new audio device into your `settings.json`, go to Mixijo again, and press `CTRL + SHIFT + R` to reload the devices.
It should print this:
```
[Mixijo] 2022-10-02 07:52:00 Reloaded settings and reopened devices
```
Once you've done that, press `CTRL + L` to list the available endpoints, that should look something like this:
```
[Mixijo] 2022-10-02 13:39:38 [log] opened audio device: Synchronous Audio Router
[Mixijo] 2022-10-02 13:39:38 [log]   inputs:
[Mixijo] 2022-10-02 13:39:38 [log]     In 1
[Mixijo] 2022-10-02 13:39:38 [log]     In 2
[Mixijo] 2022-10-02 13:39:38 [log]   outputs:
[Mixijo] 2022-10-02 13:39:38 [log]     Out 1
[Mixijo] 2022-10-02 13:39:38 [log]     Out 2
```
You then open the `settings.json` again, so you can add channels using these endpoints. 
Find the `"channels"` field, and add the output and input channels you need.
Here's an example:
```
{
    ...
    "channels" : {
        "outputs" : [
            { "endpoints" : [ "Out 1", "Out 2" ], "name" : "Output" }
        ],
        "inputs" : [
            { "endpoints" : [ "In 1" ], "name" : "Microphone" },
            { "endpoints" : [ "In 2" ], "name" : "Synth" }
        ]
    },
    ...
}
```
Once you added all the channels of your liking, go to Mixijo again and press `CTRL + R` to reload the settings.
You should then see your channels appear! You can then hide the console window using `CTRL + C`, if you press that again it will reappear.

## Shortkeys
I couldn't be bothered to add buttons to the UI, so there's some shortkeys for certain actions:

`CTRL + S` Save the current routing (also happens on close and when you modify the routing)

`CTRL + SHIFT + R` Refresh the settings and reopen all the device

`CTRL + R` Refresh settings without reopening all the devices

`CTRL + C` Hide/show the console window

`CTRL + I` Open the ASIO control panel

`CTRL + L` List information about the current device

# Settings
Mixijo uses a `settings.json` to control your channel configuration, audio device, midi device, samplerate, etc.
This is an example:
```json
{
    "audio": "Synchronous Audio Router",
    "samplerate": 48000,
    "buffersize": 32,
    "midiin": "Arturia KeyLab Essential 49",
    "midiout": "loopMIDI Port",
    "buttons": [ ],
    "channels" : {
        "outputs" : [
            { "endpoints" : [ "OBS 1", "OBS 2" ], "name" : "OBS", "midimapping" : [] },
            { "endpoints" : [ "FLStudio 1", "FLStudio 2" ], "name" : "FLStudio", "midimapping" : [] },
            { "endpoints" : [ "Ableton 1", "Ableton 2" ], "name" : "Ableton", "midimapping" : [] },
            { "endpoints" : [ "Discord 1", "Discord 2" ], "name" : "Discord", "midimapping" : [] },
            { "endpoints" : [ "Output 1", "Output 2" ], "name" : "Output", "midimapping" : [ { "cc" : 73, "param" : "gain" } ] }
        ],
        "inputs" : [
            { "endpoints" : [ "Music 1", "Music 2" ], "name" : "Music", "midimapping" : [ { "cc" : 75, "param" : "gain" } ] },
            { "endpoints" : [ "Input 1" ], "name" : "Modular", "midimapping" : [ { "cc" : 79, "param" : "gain" } ] },
            { "endpoints" : [ "FLStudio 1", "FLStudio 2" ], "name" : "FLStudio", "midimapping" : [ { "cc" : 72, "param" : "gain" } ] },
            { "endpoints" : [ "Discord 1", "Discord 2" ], "name" : "Discord", "midimapping" : [ { "cc" : 80, "param" : "gain" } ] },
            { "endpoints" : [ "System 1", "System 2" ], "name" : "System", "midimapping" : [ { "cc" : 81, "param" : "gain" } ] },
            { "endpoints" : [ "Ableton 1", "Ableton 2" ], "name" : "Ableton", "midimapping" : [ { "cc" : 82, "param" : "gain" } ] },
            { "endpoints" : [ "Chrome 1", "Chrome 2" ], "name" : "Chrome", "midimapping" : [ { "cc" : 83, "param" : "gain" } ] },
            { "endpoints" : [ "Input 2" ], "name" : "Mic", "midimapping" : [ { "cc" : 85, "param" : "gain" } ] }
        ]
    },
    "theme": {  }
}
```

`audio`: The name of the ASIO device.

`midiin`: The midi device you want to use to control the midi mappings of the channels with.

`midiout`: Useful when you have a virtual midi device, Mixijo simply forwards all midi messages to this output.

`buttons`: You can link buttons on your midi keyboard to batch files, we'll get to this later!

`channels`: All your channels, this is divided into output channels and input channels

`theme`: You can make a custom them! We'll get to this later!

## Link Midi
First you need to select your midi input device in `settings.json`:

```
{
   ...
   "midiin" : "My Midi Device",
   ...
}
```
Once you've saved it, go back to Mixijo and press `CTRL + SHIFT + R` to reload the devices.
You can then map midi controls to parameters, currently only the gain of a channel can be mapped.
In a channel in your `settings.json`, add a `"midimapping"` field, just like this:
```
{ "endpoints" : [...], "name": "My Channel", "midimapping" : [ { "cc" : 72, "param" : "gain" } ] }
```
Modify the `"cc"` field to the identifier of the midi control you want to use.

Often you also want to use the same midi device inside your DAW, that's why there's also a `"midiout"` device.
Mixijo will simply forward all the messages from the midi-in device to the midi-out device. 
This can be useful if you have a virtual midi device like [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html).

```
{
   ...
   "midiout" : "My Midi Device",
   ...
}
```

## Custom Buttons
In the buttons array of the json you can link midi buttons to executable files, here's an example:
```json
"buttons" : [ { "cc" : 36, "run": ".\\scripts\\my_script.bat" } ]
```

## Custom Theme
You can customize pretty much all the colors of Mixijo. Here are all the properties you can modify:
```
background
divider
border
close.background
close.icon
minimize.background
minimize.icon
maximize.background
maximize.icon
routebutton.background
routebutton.border
routebutton.borderWidth
channel.background
channel.slider
channel.meter
channel.meterBackground
channel.border
channel.meterLine1
channel.meterLine2
channel.meterText
channel.title
channel.value
channel.borderWidth
```
Most of those properties are also linked to state, so you can define a different color for `hovering`, `focused`, `pressed`, `selected`, and `disabled`.
Colors are encoded in json as an array of integers: `[gray]`, `[gray, alpha]`, `[r, g, b]`, `[r, g, b, alpha]`.
Here's an example of a custom theme:
```json
"theme" : {
    "background" : [ 55 ],
    "divider" : [ 65 ],
    "border" : [ 40 ],
    "channel" : {
        "background": { "transition": 100, "color": [ 65 ], "selected": [ 85 ] },
        "slider": { "color": [ 140 ] },
        "meter": {
            "color" : [ 0, 255, 0 ],
            "background" : { "transition" : 200, "color": [ 30 ], "hovering": [ 40 ] },
            "text" : { "color" : [ 180 ] },
            "line1" : { "color" : [ 133 ] },
            "line2" : { "color" : [ 90 ] }
        },
        "border": {
            "transition" : 500,
            "color" : [ 55 ],
            "selected" : [ 85 ],
            "hovering" : [ 75 ],
            "width" : {
                "transition" : 100,
                "value" : 0,
                "hovering" : 90
            }
        },
        "title": { "color" : [ 200 ] },
        "value": { "color" : [ 200 ] }
    },
    "routebutton" : {
        "background" : {
            "transition" : 100,
            "color" : [ 100, 0 ],
            "disabled" : [ 100, 0 ],
            "hovering" : [ 120, 150 ],
            "selected" : [ 120 ]
        },
        "border" : {
            "color" : [ 0, 0 ],
            "width" : {
                "transition" : 100,
                "value" : 0,
                "disabled" : 0,
                "selected" : 0,
                "hovering" : 10
            }
        }
    },
    "close" : {
        "background" : {
            "transition" : 100,
            "color" : [ 255, 0, 0, 0 ],
            "pressed" : [ 170, 0, 0, 111 ],
            "hovering" : [ 255, 0, 0, 111 ]
        }
    },
    "minimize" : {
        "background" : {
            "transition" : 100,
            "color" : [ 255, 0 ],
            "pressed" : [ 255, 58 ],
            "hovering" : [ 255, 31 ]
        }
    },
    "maximize" : {
        "background" : {
            "transition" : 100,
            "color" : [ 255, 0 ],
            "pressed" : [ 170, 58 ],
            "hovering" : [ 255, 31 ]
        }
    }
}

```

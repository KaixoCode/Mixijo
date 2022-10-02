# Mixijo

Lightweight audio mixer for ASIO devices.

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

`midiout`: Useful when you have a virtual midi device, Mixijo simply forwards all midi messages from the input to this output.

`buttons`: You can link buttons on your midi keyboard to batch files, we'll get to this later!

`channels`: All your channels, this is divided into output channels and input channels

`theme`: You can make a custom them! We'll get to this later!

## Channel
A channel consists of endpoints from the ASIO device, simply put the names of all the ASIO endpoints you want in 
the channel in the endpoints array in the json, as seen in the example:
```json
"endpoints" : [ "Endpoint 1", "Endpoint 2" ]
```

You can also map parameters to midi, currently only the gain of a channel can be mapped.
```json
"midimapping" : [ { "cc" : 72, "param" : "gain" } ]
```

## Shortkeys
I couldn't be bothered to add buttons to the UI, so there's some shortkeys for certain actions:

`CTRL + S` Save the current routing (also happens on close and when you modify the routing)

`CTRL + SHIFT + R` Refresh the settings and reopen all the device

`CTRL + R` Refresh settings without reopening all the devices

`CTRL + C` Hide/show the console window

`CTRL + I` Open the ASIO control panel

`CTRL + L` List information about the current device

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

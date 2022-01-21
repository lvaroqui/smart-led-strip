# Smart Led Strip

This repository contains everything linked to the Smart Led Strip that I showcase on my french Youtube channel. You can find the playlist [here](https://www.youtube.com/watch?v=J8o0olcY1OM&list=PLjJ8MpspHf32UaKSQNDGwbeBrOrvNWHvA).

## Project description

A Smart Led Strip controllable via [home assistant](https://www.home-assistant.io/) with custom PCB, box enclosure for electronics, firmware and home assistant plugin.

## Electronics

The Led Strip used is [this one](https://www.amazon.fr/gp/product/B01D1I50UW/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&th=1), it features 4 color channels: R, G, B, Warm White.
We use the [Arduino 33 IoT](https://store-usa.arduino.cc/products/arduino-nano-33-iot) as our brain and some [BOJACK IRLZ34N MOSFET](https://www.amazon.fr/gp/product/B0893WBH6H/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1) for the power stage. Here is the schematics of the circuit:

![Schematics](Schematics.png)

## Firmware

The firmware uses [PlatformIO](https://platformio.org/) instead of the default Arduino IDE.

**IMPORTANT**: before trying to build copy [secret_example.h](src/secret_example.h) to `src/secret.h` and fill the missing data.

We expose a simple HTTP server that controls the LED. There is one allowed route: `http://<board_ip>/command` with POST method and a specific JSON querry. The querry should always have this form:

```jsonc
{
    "method": "<your-method>",
    "param": ... // Optional param for some methods
}
```

### Supported methods

The LedStrip uses floats from 0 to 1 to represent color channel intensity. When you see `"r"`, `"g"`, `"b"` or `"w"` in the table, the associated value will be a number between 0 and 1.

#### Get Status

Get the current status.

```jsonc
// Request
{
    "method": "get_status"
}

// Response
{
    "power": <0/1>,
    "r": <val>,
    "g": <val>,
    "b": <val>,
    "w": <val>
}
```

#### Get Info

Get various informations.

```jsonc
// Request
{
    "method": "get_info"
}

// Response
{
    "mac": <mac-address>
}
```

#### Set Power

Turn the power on or off.

````jsonc
// Request
{
    "method": "set_power",
    "param": {
        "value": <0/1>
    }
}

#### Set RGBW

Set a given color.

```jsonc
// Request
{
    "method": "set_rgbw",
    "param": {
        "time": <transition-time-ms>, // Optional
        "r": <val>,
        "g": <val>,
        "b": <val>,
        "w": <val>
    }
}


// No Response
````

## Box

I desisgned a small enclosing box for the electronics using [FreeCad](https://www.freecadweb.org/) and printed it on my 3D printer. The model can be found in the [box](box) folder.

## Home Assistant

I wrote a basic [Home Assistant](https://www.home-assistant.io/) plugin to control the Led Strip from a friendly user interface. It can be found in [echow_led_strip](echow_led_strip). To enable it, place the folder in the `custom_components` folder of your Home Assistant installation.

# t-deck-keyboard-hydra

This repo further expands on hasn0life's extended keyboard firmware for the Lilygo t-deck.   
Critically, it adds the ability for the main ESP32-S3 to toggle the backlight, and to set a 'raw output' mode, for firmwares that want to implement their own, more advanced keyboard features. 

This has been made specifically to enable the possibility of using the advanced features in MicroHydra's KeyBoard module (like key-repeating, global keyboard shortcuts, held keys, etc).

I used hasn0life's firmware as a jumping off point for two reasons; First being that I had difficulties actually getting the original firmware code to work at all. And second, because flashing new firmware on the T-deck's keyboard is annoying, and I imagine that most people would prefer to only have to do it once, so you might as well get the best of both worlds in one firmware :)   
The kb firmware defaults to setting 'raw output' off, to allow backwards-compatibility with other firmwares. 

<br/>

## Reference

For reference, here are the flags used for sending settings from the main controller, to the keyboard:   
  `0` - Backlight off   
  `1` - Backlight on   
  `2` - Raw output off   
  `3` - Raw output on   

And, here's the formula used for packing columns/rows into an integer for raw output:   
`rawKeys[rawIndex] = (colIndex << 4) | (rowIndex + 1)` 

<br/>

The firmware also pulses the interrupt pin to HIGH any time the keyboard would print a keystroke.

<br/>
<br/>
<br/>

That's all I have to add. Here's the rest of the original readme by hasn0life, which includes some further information about their firmware (and how to flash it):

<br/>

----


This repo adds the following features to the keyboard microcontroller in the [Lilygo t-deck](https://www.lilygo.cc/products/t-deck):
 - sym + $/speaker now makes forward slash \
 - mic key produces tilde ~
 - alt and right shift = toggle capslock
 - left shift and sym = toggle symlock
	 - space key produces space under sym/symlock
 - holding sym lets you type symbols continuously
 - long pressing a printable key lets you type a symbol
 - long pressing the backspace key lets you continuously erase

It's based on the [firmware Lilygo provides](https://github.com/Xinyuan-LilyGO/T-Deck/blob/master/examples/Keyboard_ESP32C3/Keyboard_ESP32C3.ino) and incorporates features from the [bbq20kbd](https://www.solder.party/docs/bbq20kbd/leaflet_bbq20kbd.png), while keeping the original protocol compatible (for now?)

# How to Program the Keyboard Controller

The t-deck contains an ESP32C3 configured as an I2C slave to read the keyboard inputs. It's accessed using the 6 pin connector holes below the reset button. The [t-deck repo](https://github.com/Xinyuan-LilyGO/T-Deck/tree/master) seems to describe it upside down as of this writing.

## Hardware Setup
There are many ways to program this controller, I'm going to do it using the [FTDI TTL-232R-3V3 USB to serial cable](https://ftdichip.com/products/ttl-232r-3v3/). Other USB to serial converters may also be used. Connect the pins as shown in the diagram below (TX to RX, RX to TX, GND to GND). To put the ESP32C3 into programming mode, short the BL pin to ground and power it on. 

Note: the header doesnt use standard breadboard spaced holes so you might want to use wires.

Note 2: ESP32C3 controller has to be turned on by pin 10 in the main ESP32S3, so make sure it's firmware does that (the default firmware does).
```
t-deck    |
         |=|  rst button       ftdi cable
          |                ____________
          |               | RTS         Green
        o | TX -----------| RX          Yellow
        o | RX -----------| TX          Orange
        o | BL -----|     | VCC         Red
        o | RST    /      | CTS         Brown
        o | GND ----o-----| GND         Black
        o | VCC           |_____________
          |             
-|       /
_|______/

```

## Software Settings
The Arduino IDE configuration in the tools menu:
 - Board -> ESP32C3 Dev Module
 - USB CDC On Boot -> disabled
 - CPU Frequency -> 40MHz
 - USB DFU On Boot -> Disable
 - Flash Mode -> DIO 40MHz
 - Flash Size -> 4MB(32Mb)
 - Partition Scheme -> Default 4Mb with spifs
 - Upload Speed -> 921600

Select the COM port of the USB to serial cable. Once the controller is in programming mode (it'll print a message over the UART saying "waiting for download" when powered up with the BL pin shorted to ground) press download on the Arduino IDE and it should upload the firmware. Restart the device afterwards.

# Future Additions
Feel free to suggest ideas or modify it for your own needs. 

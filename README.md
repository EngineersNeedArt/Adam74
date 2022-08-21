<p align="center">
<img width=300 src="https://github.com/EngineersNeedArt/Adam74/blob/bb2509f71e3a739192251251af161f3ef16f7350/documentation/Adam74_logo.jpg" alt="Adam74 logo.">
</p>

**Adam74** is a small ASCII terminal intended for hobbyist 8-bit computers. It has a simple, vintage-style interface: 7 pins for 7-bits of ASCII data and another "strobe" pin to tell the Adam74 to add the character to its buffer and display it. Special control characters allow cursor movement, etc. Additionally, inverted text is supported as well.

<p align="center">
<img width=512 src="https://github.com/EngineersNeedArt/Adam74/blob/298bcce5e79116f11314677929cdb8b52a4ca69a/documentation/Adam74_code_display.jpeg" alt="Adam74 displaying source code.">
</p>

Who is this for? I imagine the audience would be people tinkering with [Ben Eater](https://eater.net/8bit/) style 8-bit computers. With a simple ASCII interface, 40 columns and 24 rows of text, it is ideal as an Apple I era computer terminal.

The Adam74 consists of a PCB with a (mounted) 2.8" or 3.2" ILI9341-based LCD display driven by an onboard Teensy microcontroller. Additionally it has a small speaker (beep) and can display 40 columns by 24 rows of text. Jumpers provide additional default configuration possibilities (as an example, displaying amber-colored rather than green text). Software on the Teensy handles text buffering, scrolling — recognizes a set of control codes to position and move the cursor.

The source code is open. Also provided are the Gerber files if you would like to create the PCB, and an SVG template if you would like to create an acrylic stand.

<p align="center">
<img width=512 src="https://github.com/EngineersNeedArt/Adam74/blob/8bbee504578fed35b0cd1091fc3898ca87edf5b9/documentation/Adam74_spock.jpeg" alt="Adam74 displaying ASCII art.">
</p>

## Hardware:

Here are the parts needed to populate the PCB:

- Teensy 4.0 ([PJRC](https://www.pjrc.com/store/))
- Color 320x240 ILI9341-based LCD display ([PJRC](https://www.pjrc.com/store/) and other places)
- 74LVC245 level shifter ([Adafruit](https://www.adafruit.com/product/735) and other places)
- Buzzer ([Adafruit](https://www.adafruit.com/product/1536) and other places)
- 100 ohm, 1/4 Watt resistor (2×)
- 0.1uF ceramic capacitor (3×) ([Adafruit](https://www.adafruit.com/product/753) and other places)
- 1N4148 small signal diode ([Adafruit](https://www.adafruit.com/product/1641) and other places)
- 0.1" male header pin strip ([Adafruit](https://www.adafruit.com/product/392) and other places)
- 20 pin (2×10, 2.54mm pitch) IDC male connector (*optional*) 

Additional hardware for mounting the PCB to the acrylic stand:

- 10mm standoffs, 2.5M threads (4×)
- 2.5M screws, nuts (4×)

To create a PCB, gerber files are included than can be uploaded to a site like [PCBWay](https://www.pcbway.com).

An SVG file is also included that can be used to laser cut an acrylic stand.

<p align="center">
<img width=512 src="https://github.com/EngineersNeedArt/Adam74/blob/1e19b9d919c8cc4eeaa0b710c39277b4ce7b90de/documentation/Adam74_yum.jpeg" alt="Serveral Adam74s in differnt colors.">
</p>

## Software:

The Adam74 is built around the [Teensy](https://www.pjrc.com) microcontroller. If you are new to the Teensy, it is uses a special version of the Aurduino IDE called [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html). It is free and available from the Teensy web site.

In addition, I make use of an optimized library for talking with the ILI9341 (the chip on the LCD display) from Paul Stoffregen available from his GitHub repository [here](https://github.com/PaulStoffregen/ILI9341_t3).

#### Adam74.ino

The main file is `Adam74.ino`. It includes `ILI9341_t3.h` from the aforementioned GitHub repo as well as `TextBuffer.h` and the font header, `font_LiberationMonoBold.h`. As with most Arduino code bases, we define the eighteen or so odd GPIO pins we need at the top of the file and configure them in the `setup()` function.

Following are other defines for supported ASCII control codes as well as a few internal state contstants, time intervals, etc.

Notice that among the globals `tft` is defined and initialized. This is the object used to reference the LCD display. Also a `TextBuffer` instance is created.

In addition to defining GPIO pins, `setup()` most importantly defines an interrupt to be executed whenever the `ASCII_STROBE` goes high. This interrupt (`strobe_interrupt()`) is how the **Adam74** is alerted to new ASCII data on the ASCII bus.

Finally, `setup()` also configures the display, resets it and plays a *beep* sound to indicate the **Adam74** is ready.

The code has a kind of ring buffer for holding incoming ASCII bytes and `buffer_read` and `buffer_write` point to indicees within the buffer indicating where the last ASCII byte was written to and where within the buffer we have last read from.

And so the `loop()` function checks if `buffer_read` and `buffer_write` are out of sync. If they are, an ASCII byte is read from the buffer and processed (`handleCharacterInput()`). Additionally, `loop()` handles flashing the cursor at a specific interval.

I won't bother detailing the rest of `Adam74.ino` (I think you can figure it out) but I might point out just a few things so you dont stumble on them.

I decided to use the high bit (bit 7) of the `char` representing the character to display to indicate whether to invert the charachter when displayed. This is a bit of a hack that saved me having to have a more complex structure to keep track of which characters are inverted and which are normal. So you'll see me masking or muxing `0x80` with the character from time to time in the code.

I am not a fan of state machines on output devices like this since there is no way for the caller to know the state of the **Adam74**. Nonetheless, to handle escape sequences that are popular with terminals I had to make a consession. The default state (`ASCII_STATE`) just grabs ASCII characters from the seven ASCII pins and pushes them to the display. However, if an escape character arrives, the **Adam74** will `ESCAPE_STATE` and a rather dodgy and incomplete implementation in `handle_escape_state()` tries to handle the characters that follow.

I implemented my own simpler means to control the cursor position (rather than using the escape codes) and had to introduce my own `CURSOR_X_STATE` and `CURSOR_Y_STATE`.

#### TextBuffer.cpp

This is a straightforward class you initialize with the number of rows and columns of text you want buffered. The class then act as a container for row x col characters. Methods allow you to clear rows of text, insert text, and scroll upward.

#### font_LiberationMonoBold.c

This was about the best font I could find that was legible when very small (to get all of 24 rows by 40 columns on a 320 x 240 display the font must be legible at only 9 pixels tall and 7 pixels wide).

## How to Use:

<p align="center">
<img width=327 src="https://github.com/EngineersNeedArt/Adam74/blob/1d7cf30ec44e02675cf957338ff4297557a4ac0e/documentation/IDC_Pinout.png" alt="IDC pinout.">
</p>

The pinout when viewed from the back of the PCB is shown above. The entire bottom row of pins are ground (if you use a 20-pin IDC ribbon socket every other wire carries ground in order to isolate the signal wires).

The pins across the top row are the seven bits of ASCII (D6 to D0). The Strobe pin is how you indicate to Adam74 to ingest the 7-bit ASCII value. Adam74 indicates it is busy with the next pin, BSY. The final pin is VCC: expected to be 3.3V to 5V.

To interface to your device you supply +3.3V to +5V to VCC and connect to GND. Then connect wires to, at a minimum, the ASCII pins and STR (strobe). Set high or low the ASCII pins (D0 to D6) representing the character you wish to display on the Adam74 and then trigger the strobe by pulsing STR for 10 or so milliseconds. The character should appear on the display.

I implemented, and in some cases redefined, a number of the control codes. As you may know, the first 32 ASCII characters (fully one-quarter of the 7-bit ASCII range) are non-printing "control codes" with names like "bell", "form feed", "file seperator", etc.

Many of these control codes mapped easily to the Adam74 terminal. Bell (\<CTRL> G or 0x07 in hexidecimal) for example will trigger a short beep sound from the Adam74. Backspace (<CTRL> H or 0x08 in hexidecimal) retards the cursor by one space. Carriage Return (\<CTRL> M or 0x0D in hexidecimal) behaves as you might expect: the cursor on the Adam74 moves to the beginning of the next line.

To add more functionality some of the other control codes that did not have a clear utility were repurposed for Adam74. Start of Text  (\<CTRL> B or 0x02 in hexidecimal) is interpreted by the Adam74 to move the cursor to the home position (top-left of the display). The Form Feed (\<CTRL> L or 0x0C in hexidecimal) character will cause Adam74 to clear the entire screen.

Here is a complete table of the supported control codes and their behavior in Adam74.

| Code | Keyboard, Hex.  | Adam74          |
| ---  | ---             | ---             |
| STX  | \<CTRL> B, 0x02 | Cursor Home     |
| BEL  | \<CTRL> G, 0x07 | Beep            |
| BS   | \<CTRL> H, 0x08 | Backspace       |
| FF   | \<CTRL> L, 0x0C | Clear Screen    |
| CR   | \<CTRL> M, 0x0D | Carriage Return |
| SO   | \<CTRL> N, 0x0E | Invert Off      |
| SI   | \<CTRL> O, 0x0F | Invert On       |
| DLE  | \<CTRL> P, 0x10 | Clear Line      |
| DC1  | \<CTRL> Q, 0x11 | Cursor On       |
| DC2  | \<CTRL> R, 0x12 | Cursor Off      |
| DC3  | \<CTRL> S, 0x13 | Cursor XY*      |
| ESC  | \<CTRL> [, 0x1B | Escape          |
| FS   | \<CTRL> \\, 0x1C | Cursor Up       |
| GS   | \<CTRL> ], 0x1D | Cursor Down     |
| RS   | \<CTRL> ^, 0x1E | Cursor Left     |
| US   | \<CTRL> _, 0x1F | Cursor Right    |

\* after sending the Cursor XY command the next two "characters" are interpreted as the X and then Y position of the cursor.

I provided three jumpers on the back of the Adam74. Currently only `JMPR0` is implemented. Jumpering it will cause the Adam74 to boot to display amber(ish) colored text rather than the default green.

In the future it would be nice to define another jumper to configure the Adam74 in "portrait" rather than "landscape" mode (with of course a different row/column count).

To generate the images seen in the photos here, I wrote a simple test app on a second Teensy to send ASCII characters to the Adam74. The code to send an ASCII character looks like this:

```
void sendASCII (char ascii) {
    // Set ASCIII bits on the bus (our 7 ASCII output pins)
    digitalWrite (D0_PIN, ascii & 0x01);
    ascii = ascii >> 1;
    digitalWrite (D1_PIN, ascii & 0x01);
    ascii = ascii >> 1;
    digitalWrite (D2_PIN, ascii & 0x01);
    ascii = ascii >> 1;
    digitalWrite (D3_PIN, ascii & 0x01);
    ascii = ascii >> 1;
    digitalWrite (D4_PIN, ascii & 0x01);
    ascii = ascii >> 1;
    digitalWrite (D5_PIN, ascii & 0x01);
    ascii = ascii >> 1;
    digitalWrite (D6_PIN, ascii & 0x01);
    
    // Kick off strobe: set pin high, wait, set pin low.
    digitalWrite (STRB_PIN, HIGH);
    delayMicroseconds (STRB_MICROSECS);   // 150 microseconds
    digitalWrite (STRB_PIN , LOW);
}
```
As you can see, it is a simple matter of setting high or low the pins corresponding to the ASCII bits and then follow that by bringing the STRB pin high and then low. The software on the Adam74 looks for a rising edge on the STRB pin. That will trigger the Adam74 to scoop the ASCII bits off the bus and display the character.

It's not clear to me what the ideal delay for the strobe is. Originally I had a 300 microsecond delay but found 150 worked fine as well. You may be able to make it even more brief.

## Improvements:

I wish scrolling were faster. I have experimented briefly with ILI9341 hardware scrolling but if I recall correctly, I could not get it to scroll vertically. Without hardware scrolling, all 40 x 24 characters have to be redrawn to the display when scrolling happens. This performance bottleneck in fact was why I went with the Teensy 4.0 rather than one of the less expensive but slower Teensy models.
  
If hardware scrolling is not possible, perhaps there are other optimizations that could be made to make rendering more performant.

Other features like ANSI colors would be cool as well. I'm afraid I don't know enough about terminal protocols to implement something like this correctly.

More detail on creating the Adam74 [on my blog](https://www.engineersneedart.com/adam74/adam74.html).

> I have a half dozen or so Adam74s that I built for the photos. If you are interested in one I would have to ask $140 (plus shipping). These were surprisingly expensive with the minimum orders and shipping costs for laser-cutting, PCB-fabbing, etc.

> If you are interested in one of the completed Adam74s email me. I am also considering a "thin kit" of sorts (PCB + acrylic stand) to get the cost down. Let me know if that is interesting as well. Below is my *hide-my* email address:

<p align="center">
<img width=184 src="https://github.com/EngineersNeedArt/Adam74/blob/4fc9f79ab609d0990342e8c9c20aaae8bbc1e597/documentation/email_image.png" alt="Me email address.">
</p>


<p align="center">
<i>"Good enough for 1.0…"</i>
</p>

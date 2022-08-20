// ==============================================================================
// Adam74 (Teensy4.0)
// ==============================================================================

#include <ILI9341_t3.h>
#include "TextBuffer.h"
#include "font_LiberationMonoBold.h"


// #define DEBUG

// Pin defines.
#define ASCII_B0                2
#define ASCII_B1                3
#define ASCII_B2                4
#define ASCII_B3                5
#define ASCII_B4                6
#define ASCII_B5                7
#define ASCII_B6                8
#define TFT_DC                  9
#define TFT_CS                  10
#define TFT_MOSI                11
#define TFT_MISO                12
#define TFT_SCK                 13
#define ASCII_STROBE            14
#define PIEZO                   19
#define BUSY_PIN                20
#define JUMPER_0                21
#define JUMPER_1                22
#define JUMPER_2                23

#define CONTROL_CURSOR_HOME     0x02    // 'start of text', <CTRL> B.
#define CONTROL_BELL            0x07    // 'bell', <CTRL> G.
#define BACKSPACE               0x08    // 'backspace', <CTRL> H.
#define CONTROL_CLEAR_SCREEN    0x0C    // 'form feed', <CTRL> L.
#define CARRIAGE_RETURN         0x0D    // 'carriage return', <CTRL> M.
#define CONTROL_INVERT_OFF      0x0E    // 'shift out', <CTRL> N.
#define CONTROL_INVERT_ON       0x0F    // 'shift in', <CTRL> O.
#define CONTROL_CLEAR_LINE      0x10    // 'data link escape', <CTRL> P.
#define CONTROL_CURSOR_ON       0x11    // 'device control 1', <CTRL> Q.
#define CONTROL_CURSOR_OFF      0x12    // 'device control 2', <CTRL> R.
#define CONTROL_CURSOR_XY       0x13    // 'device control 3', <CTRL> S.
#define CONTROL_ESCAPE          0x1B    // 'escape', <CTRL> [.
#define CONTROL_CURSOR_UP       0x1C    // 'file separator', <CTRL> \.
#define CONTROL_CURSOR_DOWN     0x1D    // 'group separator', <CTRL> ].
#define CONTROL_CURSOR_LEFT     0x1E    // 'record separator', <CTRL> ^.
#define CONTROL_CURSOR_RIGHT    0x1F    // 'unit separator', <CTRL> _.

#define CURSOR_FLASH_INTERVAL   500
#define ASCII_RING_BUFFER_SIZE  960

#define CHAR_X_MARGIN           1
#define CHAR_Y_MARGIN           1

// State machine constants.
#define ASCII_STATE             0
#define ESCAPE_STATE            1
#define CURSOR_X_STATE          2
#define CURSOR_Y_STATE          3

#define PARAMETER_1_STATE       0
#define PARAMETER_2_STATE       1

ILI9341_t3 tft = ILI9341_t3 (TFT_CS, TFT_DC);
TextBuffer text = TextBuffer ();
bool invert = false;
bool cursor_state = false;
uint8_t show_cursor = true;
int16_t cursor_x = 0;
int16_t cursor_y = 0;
int16_t num_columns = 40;
int16_t num_rows = 24;
uint8_t x_advance = 8;
uint8_t y_advance = 10;
uint16_t foreground_color = ILI9341_GREEN;
uint16_t background_color = ILI9341_BLACK;
unsigned long cursor_timer = 0;
int16_t state = ASCII_STATE;
int16_t parameterState = PARAMETER_1_STATE;
uint16_t csiParameter1 = 0;
uint16_t csiParameter2 = 0;
ILI9341_t3_font_t font = LiberationMono_8_Bold;
volatile char ascii_ring_buffer[ASCII_RING_BUFFER_SIZE];
volatile uint16_t buffer_write = 0;
volatile uint16_t buffer_read = 0;

void setup () {
#ifdef DEBUG
    Serial.begin (115200);
#endif  // DEBUG
    
    // ASCII-Bus input.
    pinMode (ASCII_B0, INPUT_PULLDOWN);
    pinMode (ASCII_B1, INPUT_PULLDOWN);
    pinMode (ASCII_B2, INPUT_PULLDOWN);
    pinMode (ASCII_B3, INPUT_PULLDOWN);
    pinMode (ASCII_B4, INPUT_PULLDOWN);
    pinMode (ASCII_B5, INPUT_PULLDOWN);
    pinMode (ASCII_B6, INPUT_PULLDOWN);
    
    // Other pins.
    pinMode (ASCII_STROBE, INPUT_PULLDOWN);
    attachInterrupt (digitalPinToInterrupt (ASCII_STROBE), strobe_interrupt, RISING);
    pinMode (PIEZO, OUTPUT);
    pinMode (BUSY_PIN, OUTPUT);
    pinMode (JUMPER_0, INPUT_PULLUP);
    pinMode (JUMPER_1, INPUT_PULLUP);
    pinMode (JUMPER_2, INPUT_PULLUP);
    
    // Set up TFT display.
    tft.begin (/*60000000*/);
    delay (10);
    tft.fillScreen (background_color);
    
    // Set up text dimensions, text buffer.
    set_display_mode (0);
    
    // Jumper 0 will determine whether this is a green or amber display.
    foreground_color = digitalRead (JUMPER_0) ? ILI9341_GREEN : ILI9341_ORANGE;
    
    // Ready.
    digitalWrite (BUSY_PIN, false);    
    beep_sound ();
    
#ifdef DEBUG
    Serial.println ("Ready");
#endif  // DEBUG
}

void loop (void) {
    // Handle incoming ASCII.
    if (buffer_read != buffer_write) {
        char newchar = ascii_ring_buffer[buffer_read];
        digitalWrite (BUSY_PIN, false);
        handleCharacterInput (newchar);
        buffer_read = buffer_read + 1;
        if (buffer_read >= ASCII_RING_BUFFER_SIZE) {
            buffer_read = 0;
        }
    }
    
    // Flash cursor if it is time.
    if (millis () > cursor_timer) {
        cursor_timer = millis () + CURSOR_FLASH_INTERVAL;
        flash_cursor ();
    }
}

void set_display_mode (int mode) {
    // BOGUS: 'mode' param ignored for now, may introduce other display modes (like portrait?).
    num_columns = 40;
    num_rows = 24;
    x_advance = 8;
    y_advance = 10;
    
    // Establish size of text buffer.
    text.begin (num_rows, num_columns);
    
    tft.setFont (font);
    tft.setRotation (3);
}

void display_char (char character, int16_t x, int16_t y, int16_t color) {
    tft.setCursor (x, y);
    tft.setTextColor (color);
    tft.drawFontChar (character);
}

void refresh_display () {
    int16_t r;
    for (r = 0; r < num_rows; r++) {
        char *charPtr = text.getRowPtr (r);
        int16_t y = r * y_advance;
        tft.fillRect (0, y, x_advance * num_columns, y_advance, background_color);
        int16_t c;
        int16_t last_char = num_columns - 1;
        for (c = 0; c <= last_char; c++) {
            char character = text.getCharRowColumn (r, c);
            bool draw_inverted = false;
            if (character >= 0x80) {
                // We add 0x80 to characters we want to draw inverted.
                character = character - 0x80;
                draw_inverted = true;
            }
            if (draw_inverted) {
                tft.fillRect (c * x_advance, y, x_advance, y_advance, foreground_color);
                display_char (character, (c * x_advance) + CHAR_X_MARGIN, y + CHAR_Y_MARGIN, background_color);
            } else {
                display_char (character, (c * x_advance) + CHAR_X_MARGIN, y + CHAR_Y_MARGIN, foreground_color);
            }
            charPtr++;
        }
    }
}

void clear_cursor () {
    if (!show_cursor) {
        return;
    }
    char character = text.getCharRowColumn (cursor_y, cursor_x);
    bool inverted_char = false;
    if (character >= 0x80) {
        // We add 0x80 to characters we want to draw inverted.
        character = character - 0x80;
        inverted_char = true;
    }
    tft.fillRect (cursor_x * x_advance, cursor_y * y_advance, x_advance, y_advance, inverted_char ? foreground_color : background_color);
    if (character != ' ') {
        display_char (character, (cursor_x * x_advance) + CHAR_X_MARGIN, (cursor_y * y_advance) + CHAR_Y_MARGIN, invert ? background_color : foreground_color);
    }
}

void reset_cursor_state () {
    cursor_state = true;
    cursor_timer = millis ();
}

void flash_cursor () {
    if (!show_cursor) {
        return;
    }
    cursor_timer = millis () + CURSOR_FLASH_INTERVAL;
    char character = text.getCharRowColumn (cursor_y, cursor_x);
    bool inverted_char = false;
    if (character >= 0x80) {
        // We add 0x80 to characters we want to draw inverted.
        character = character - 0x80;
        inverted_char = true;
    }
    tft.fillRect (cursor_x * x_advance, cursor_y * y_advance, x_advance, y_advance, inverted_char ^ cursor_state ? foreground_color : background_color);
    if (character != ' ') {
        display_char (character, (cursor_x * x_advance) + CHAR_X_MARGIN, (cursor_y * y_advance) + CHAR_Y_MARGIN, inverted_char ^ cursor_state ? background_color : foreground_color);
    }
    cursor_state = !cursor_state;
}

void handleCharacterInput (char character) {
#ifdef DEBUG
    Serial.println (character, HEX);
#endif  // DEBUG
    
    switch (state)
    {
        case ESCAPE_STATE:
        // Handle 'escape state' (see: https://en.wikipedia.org/wiki/ANSI_escape_code).
        handle_escape_state (character);
        break;
        
        case CURSOR_X_STATE:
        // Expecting cursor 'x' position.
        handle_cursor_x_state (character);
        break;
        
        case CURSOR_Y_STATE:
        // Expecting cursor 'y' position.
        handle_cursor_y_state (character);
        break;
        
        default:    // ASCII_STATE
        handle_ascii_state (character);
        break;
    }
}

/// BOGUS: escape handling is a work-in-progress (and likely incorrect/not-spec).

void handle_escape_state (char character) {
    if ((character >= 0x30) && (character <= 0x3F)) {
        // Parameter byte.
        if (parameterState == PARAMETER_1_STATE) {
            if (csiParameter1 > 0) {
                // Already fetched a parameter byte, assume multiple digits.
                csiParameter1 = (csiParameter1 << 4) + (uint16_t) character;
            } else {
                // Assign parameter byte.
                csiParameter1 = (uint16_t) character;
            }
        } else {
            if (csiParameter2 > 0) {
                // Already fetched a parameter byte, assume multiple digits.
                csiParameter2 = (csiParameter2 << 4) + (uint16_t) character;
            } else {
                // Assign parameter byte.
                csiParameter2 = (uint16_t) character;
            }                
        }
    } else if ((character >= 0x40) && (character <= 0x7E)) {
        // Final byte.
        if ((character == 'A') || (character == 'F')) {            // Cursor Up, Cursor Previous Line.
            if (csiParameter1 == 0) {
                csiParameter1 = 1;
            }
            cursor_y = cursor_y - csiParameter1;
            if (cursor_y < 0) {
                cursor_y = 0;
            }
            if (character == 'F') {
                cursor_x = 0;
            }
        } else if ((character == 'B') || (character == 'E')) {    // Cursor Down, Cursor Next Line.
            if (csiParameter1 == 0) {
                csiParameter1 = 1;
            }
            cursor_y = cursor_y + csiParameter1;
            if (cursor_y >= num_rows) {
                cursor_y = num_rows - 1;
            }
            if (character == 'E') {
                cursor_x = 0;
            }
        } else if (character == 'C') {                            // Cursor Forward.
            if (csiParameter1 == 0) {
                csiParameter1 = 1;
            }
            cursor_x = cursor_x + csiParameter1;
            if (cursor_x >= num_columns) {
                cursor_x = num_columns - 1;
            }
        } else if (character == 'D') {                            // Cursor Back.
            if (csiParameter1 == 0) {
                csiParameter1 = 1;
            }
            cursor_x = cursor_x - csiParameter1;
            if (cursor_x < 0) {
                cursor_x = 0;
            }
        } else if (character == 'G') {                            // Cursor Horizontal Absolute.
            if (csiParameter1 == 0) {
                csiParameter1 = 1;
            }
            // Command is one-based, cursor is zero-based.
            cursor_x = csiParameter1 - 1;
            if (cursor_x < 0) {
                cursor_x = 0;
            } else if (cursor_x >= num_columns) {
                cursor_x = num_columns - 1;
            }
        } else if (character == 'H') {                            // Cursor Position.
            if (csiParameter1 == 0) {
                csiParameter1 = 1;
            }
            if (csiParameter2 == 0) {
                csiParameter2 = 1;
            }
            // Command is one-based, cursor is zero-based.
            cursor_y = csiParameter1 - 1;
            if (cursor_y < 0) {
                cursor_y = 0;
            } else if (cursor_y >= num_rows) {
                cursor_y = num_rows - 1;
            }
            // Command is one-based, cursor is zero-based.
            cursor_x = csiParameter2 - 1;
            if (cursor_x < 0) {
                cursor_x = 0;
            } else if (cursor_x >= num_columns) {
                cursor_x = num_columns - 1;
            }
        } else if (character == ";") {
            // BOGUS: undefined if already in PARAMETER_2_STATE.
            parameterState = PARAMETER_2_STATE;
        }
    }
}

void handle_cursor_x_state (char character) {
    cursor_x = (int16_t) character;
    if (cursor_x < 0) {
        cursor_x = 0;
    } else if (cursor_x >= num_columns) {
        cursor_x = num_columns - 1;
    }
    
    // Advance state, expecting cursor 'y' position next.
    state = CURSOR_Y_STATE;    
}

void handle_cursor_y_state (char character) {
    cursor_y = (int16_t) character;
    if (cursor_y < 0) {
        cursor_y = 0;
    } else if (cursor_y >= num_rows) {
        cursor_y = num_rows - 1;
    }
    
    // Return to ASCII state (the default state).
    state = ASCII_STATE;
}

void handle_ascii_state (char character) {
    if (character == CONTROL_CURSOR_HOME) {
        clear_cursor ();
        home_cursor ();
        reset_cursor_state ();
    } else if (character == CONTROL_BELL) {
        beep_sound ();
    } else if (character == BACKSPACE) {
        clear_cursor ();
        retard_cursor ();
        text.deleteLastCharInRow (cursor_y);
        tft.fillRect (cursor_x * x_advance, cursor_y * y_advance, x_advance, y_advance, background_color);
        reset_cursor_state ();
    } else if (character == CONTROL_CLEAR_SCREEN) {
        invert = false;
        tft.fillScreen (background_color);
        home_cursor ();
        text.clear ();
        reset_cursor_state ();
    } else if (character == CARRIAGE_RETURN) {
        clear_cursor ();
        if (carriage_return_cursor ()) {
            refresh_display ();
        }
        reset_cursor_state ();
    } else if (character == CONTROL_INVERT_OFF) {
        invert = false;
    } else if (character == CONTROL_INVERT_ON) {
        invert = true;
    } else if (character == CONTROL_CLEAR_LINE) {
        tft.fillRect (0, cursor_y * y_advance, x_advance * num_columns, y_advance, background_color);
        cursor_x = 0;
        text.clearRow (cursor_y);
        reset_cursor_state ();
    } else if (character == CONTROL_CURSOR_ON) {
        show_cursor = true;
    } else if (character == CONTROL_CURSOR_OFF) {
        clear_cursor ();
        show_cursor = false;
    } else if (character == CONTROL_CURSOR_XY) {
        state = CURSOR_X_STATE;
    } else if (character == CONTROL_ESCAPE) {
        state = ESCAPE_STATE;   // BOGUS: undefined if already in an Escaping Sequence.
        csiParameter1 = 0;
        csiParameter2 = 0;
        parameterState = PARAMETER_1_STATE;
    } else if (character == CONTROL_CURSOR_UP) {
        clear_cursor ();
        if (cursor_y > 0) {
            cursor_y = cursor_y - 1;
        }
        reset_cursor_state ();
    } else if (character == CONTROL_CURSOR_DOWN) {
        clear_cursor ();
        if (cursor_y < (num_columns - 1)) {
            cursor_y = cursor_y + 1;
        }        
        reset_cursor_state ();
    } else if (character == CONTROL_CURSOR_LEFT) {
        clear_cursor ();
        if (cursor_x > 0) {
            cursor_x = cursor_x - 1;
        }
        reset_cursor_state ();
    } else if (character == CONTROL_CURSOR_RIGHT) {
        clear_cursor ();
        if (cursor_x < (num_columns - 1)) {
            cursor_x = cursor_x + 1;
        }
        reset_cursor_state ();
    } else if ((character >= 0x20) && (character <= 0x7F)) {
        text.setCharRowColumn (invert ? character + 0x80 : character, cursor_y, cursor_x);
        tft.fillRect (cursor_x * x_advance, cursor_y * y_advance, x_advance, y_advance, invert ? foreground_color: background_color);
        display_char (character, (cursor_x * x_advance) + CHAR_X_MARGIN, (cursor_y * y_advance) + CHAR_Y_MARGIN, invert ? background_color : foreground_color);
        if (advance_cursor ()) {
            refresh_display ();
        }
        reset_cursor_state ();
    }
}

void beep_sound () {
    tone (PIEZO, 1760, 60);
    delay(80);
    tone (PIEZO, 880, 100);
}

void home_cursor () {
    cursor_x = 0;
    cursor_y = 0;
}

bool advance_cursor () {
    bool did_scroll = false;
    cursor_x = cursor_x + 1;
    if (cursor_x >= num_columns) {
        cursor_x = 0;
        cursor_y = cursor_y + 1;
        if (cursor_y >= num_rows) {
            text.scrollUp ();
            cursor_y = num_rows - 1;
            did_scroll = true;
        }
    }
    return did_scroll;
}

void retard_cursor () {
    cursor_x = cursor_x - 1;
    if (cursor_x < 0) {
        cursor_y = cursor_y - 1;
        if (cursor_y < 0) {
            cursor_y = 0;
        }
        cursor_x = text.getLastCharIndexForRow (cursor_y) + 1;
    }
}

bool carriage_return_cursor () {
    bool did_scroll = false;
    cursor_x = 0;
    cursor_y = cursor_y + 1;
    if (cursor_y >= num_rows) {
        text.scrollUp ();
        cursor_y = num_rows - 1;
        did_scroll = true;
    }
    return did_scroll;
}

// Interrupt handler: pulls 7-bit ASCII off the bus.
// Calls handler function.

void strobe_interrupt () {
    digitalWrite (BUSY_PIN, true);
    
    char new_char = digitalRead (ASCII_B6);
    new_char = (new_char << 1) | digitalRead (ASCII_B5);
    new_char = (new_char << 1) | digitalRead (ASCII_B4);
    new_char = (new_char << 1) | digitalRead (ASCII_B3);
    new_char = (new_char << 1) | digitalRead (ASCII_B2);
    new_char = (new_char << 1) | digitalRead (ASCII_B1);
    new_char = (new_char << 1) | digitalRead (ASCII_B0);
    
    uint16_t new_buffer_write = buffer_write + 1;
    if (new_buffer_write >= ASCII_RING_BUFFER_SIZE) {
        new_buffer_write = 0;
    }
    
    // Sanity check so our write-head doesn't pass our read-head in our circular buffer.
    if (new_buffer_write == buffer_read) {
        // Collision.
        return;
    }
    
    ascii_ring_buffer[buffer_write] = new_char;
    buffer_write = new_buffer_write;
}
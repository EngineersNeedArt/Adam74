// ==============================================================================
// TextBuffer.cpp
// ==============================================================================


#include "TextBuffer.h"


TextBuffer::TextBuffer () {
    text_buffer = NULL;
    buffer_last_char = NULL;
    num_rows = 0;
    num_columns = 0;
}

TextBuffer::~TextBuffer () {
    freeBuffer();
}

void TextBuffer::freeBuffer () {
    int16_t r;
    free (buffer_last_char);
    for (r = 0; r < num_rows; r++) {
        free (text_buffer[r]);
    }
    free (text_buffer);
    
    text_buffer = NULL;
    buffer_last_char = NULL;
    num_rows = 0;
    num_columns = 0;    
}

void TextBuffer::begin (uint16_t rows, uint16_t columns) {
    // Tear down malloc'd memory if already created.
    if (text_buffer) {
        freeBuffer();
    }
    
    int16_t r;
    num_rows = rows;
    num_columns = columns;
    text_buffer = (char **) malloc (num_rows * sizeof (char *));
    
    for (r = 0; r < num_rows; r++) {
        // Add extra space for a NULL terminator.
        text_buffer[r] = (char *) malloc ((num_columns + 1) * sizeof (char));
    }
    
    buffer_last_char = (int16_t *) malloc (num_rows * sizeof (int16_t));
    
    clear ();
}

bool TextBuffer::clearRow (int16_t row) {
    // Param check.
    if ((row < 0) || (row >= num_rows)) {
        return false;
    }
    
    char *charPtr = text_buffer[row];
    int16_t c;
    for (c = 0; c < num_columns; c++) {
        *charPtr = ' ';
        charPtr++;
    }
    // Append NULL terminator.
    *charPtr = 0;
    
    // Indicate no (non-space) characters in this row.
    buffer_last_char[row] = -1;
    
    return true;
}

void TextBuffer::clear () {
    int16_t r;
    
    // Walk each row and clear it.
    for (r = 0; r < num_rows; r++) {
        clearRow (r);
    }
}

bool TextBuffer::setStringForRow (char *string, int16_t row) {
    // Param check.
    if ((row < 0) || (row >= num_rows)) {
        return false;
    }
    
    // Copy over no more than 'num_columns' characters.
    size_t length = min ((size_t) num_columns, strlen (string));
    
    strncpy (text_buffer[row], string, length);
    
    // BOGUS: technically not correct since 'string' could have trailing spaces.
    buffer_last_char[row] = length - 1;
    
    return true;
}

bool TextBuffer::setCharRowColumn (char character, int16_t row, int16_t column) {
    // Param check.
    if (((row < 0) || (row >= num_rows)) || ((column < 0) || (column >= num_columns))) {
        return false;
    }
    
    // Insert character.
    char *charPtr = text_buffer[row];
    charPtr += column;
    *charPtr = character;
    
    // Bump last (non-space) characters in this row.
    if ((character != ' ') && (column > buffer_last_char[row])) {
    // if (column > buffer_last_char[row]) {
        buffer_last_char[row] = column;
    }
    
    return true;
}

char TextBuffer::getCharRowColumn (int16_t row, int16_t column) {
    // Param check.
    if (((row < 0) || (row >= num_rows)) || ((column < 0) || (column >= num_columns))) {
        return 0;
    }
    
    // Get char.
    char *charPtr = text_buffer[row];
    charPtr += column;
    return (char) *charPtr;
}

int16_t TextBuffer::getLastCharIndexForRow (int16_t row) {
    // Param check.
    if ((row < 0) || (row >= num_rows)) {
        return -1;
    }
    
    int16_t index = buffer_last_char[row];
    while ((index >= 0) && (getCharRowColumn (row, index) == ' ')) {
        index = index - 1;
    }
    return index;
}

char *TextBuffer::getRowPtr (int16_t row) {
    // Param check.
    if ((row < 0) || (row >= num_rows)) {
        // throw RangeError;
        row = 0;    // BOGUS, should figure out how to throw.
    }
    
    return (char *) text_buffer[row];
}

bool TextBuffer::deleteLastCharInRow (int16_t row) {
    // Param check.
    if ((row < 0) || (row >= num_rows)) {
        return false;
    }
    
    int16_t last_char = buffer_last_char[row];
    if ((last_char < 0) || (last_char >= (num_columns - 1))) {
        return false;
    }
    
    char *charPtr = text_buffer[row];
    charPtr += last_char;
    *charPtr = ' ';
    last_char = last_char - 1;
    buffer_last_char[row] = last_char;
    return true;
}

void TextBuffer::scrollUp () {
    int16_t r;
    
    for (r = 0; r < num_rows - 1; r++) {
        // Copy string.
        // strcpy (text_buffer[r], text_buffer[r + 1]);
        memcpy (text_buffer[r], text_buffer[r + 1], sizeof(char) * (num_columns + 1));
        
        // Copy last (non-space) character indicator.
        buffer_last_char[r] = buffer_last_char[r + 1];
    }
    
    // Clear last row.
    clearRow (num_rows - 1);
}

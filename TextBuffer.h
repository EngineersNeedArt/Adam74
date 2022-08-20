// ==============================================================================
// TextBuffer.h
// ==============================================================================


#ifndef _TextBuffer_
#define _TextBuffer_

#ifdef __cplusplus
#include "Arduino.h"
#endif

#ifdef __cplusplus

class TextBuffer {
public:
    /// Create/destroy TextBuffer object.
    TextBuffer ();
    ~TextBuffer ();
    
    /// Establishes row and column size of TextBuffer. Calling this resets/clears the buffer.
    void begin (uint16_t rows, uint16_t columns);
    
    /// Clears the text buffer (buffer is populated with space characters).
    void clear ();
    
    /// Clears 'row' of  text buffer (row is populated with space chars).
    /// Returns 'false' if 'row' is out of range.
    bool clearRow (int16_t row);
    
    /// Assigns all chars from 'string' to 'row' of the text buffer.
    /// If strlen(string) exceeds the maximum number of characters for a row, it is truncated to length.
    /// Returns 'false' if 'row' is out of range.
    bool setStringForRow (char *string, int16_t row);
    
    /// Assigns 'character' to the 'row' and 'column' specified.
    /// Returns 'false' if 'row' or 'column' are out of range.
    bool setCharRowColumn (char character, int16_t row, int16_t column);
    
    /// Given 'row' and 'column' returns the character from the buffer in this location.
    char getCharRowColumn (int16_t row, int16_t column);
    
    /// Returns the index of the last non-space char inm 'row'. 
    /// Returns -1 for a clear row (a row with only spaces).
    int16_t getLastCharIndexForRow (int16_t row);
    
    /// Returns a pointer to 'row' buffer. WARNING: buffer returned should be treated read-only.
    char *getRowPtr (int16_t row);
    
    /// Deletes the last character in 'row'.
    /// Returns 'false' if 'row' is out of range.
    bool deleteLastCharInRow (int16_t row);
    
    /// Scrolls each row of chars up. Top row is lost, bottom row is clear (all space chars).
    void scrollUp ();
    
private:
    char **text_buffer;
    int16_t *buffer_last_char;
    uint16_t num_rows;
    uint16_t num_columns;
    
    void freeBuffer ();
};

#endif  // __cplusplus

#endif  // _TextBuffer_
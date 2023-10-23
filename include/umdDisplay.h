#ifndef UMD_DISPLAY_H
#define UMD_DISPLAY_H

#include <sstream>
#include <Adafruit_SSD1306.h>

#define OLED_RESET -1 
#define OLED_SCREEN_WIDTH 128
#define OLED_SCREEN_HEIGHT 64

#define OLED_FONT_WIDTH 6
#define OLED_FONT_HEIGHT 8

#define OLED_MAX_CHARS_PER_LINE         OLED_SCREEN_WIDTH / OLED_FONT_WIDTH
#define OLED_MAX_LINES_PER_SCREEN       OLED_SCREEN_HEIGHT / OLED_FONT_HEIGHT
#define OLED_CHAR_NUMBER(n)             OLED_FONT_WIDTH * n
#define OLED_LINE_NUMBER(n)             OLED_FONT_HEIGHT * n

#define UMD_DISPLAY_BUFFER_CHARS_PER_LINE   OLED_MAX_CHARS_PER_LINE*2
#define UMD_DISPLAY_BUFFER_TOTAL_LINES      32

class UMDDisplay
{
    public:
        int scroll[OLED_MAX_LINES_PER_SCREEN][2]; // starting character for each line

        UMDDisplay();
        bool begin(Adafruit_SSD1306 *display);
        void clear(void);
        void clearLine(int lineNumber);
        void print(std::stringstream stringStream, int lineNumber, int pos = 0);
        void refresh();

    private:
        Adafruit_SSD1306 *_display;
        char buffer[UMD_DISPLAY_BUFFER_CHARS_PER_LINE][UMD_DISPLAY_BUFFER_TOTAL_LINES];
};

#endif
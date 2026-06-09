#ifndef __UI_FONT_H
#define __UI_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
  char code;
  uint8_t width;
  uint8_t height;
  int8_t x_offset;
  int8_t y_offset;
  uint8_t x_advance;
  const uint8_t *coverage;
} FontGlyph_t;

typedef struct
{
  uint8_t line_height;
  uint8_t baseline;
  uint8_t glyph_count;
  const FontGlyph_t *glyphs;
} Font_t;

extern const Font_t UIFont_LargeNumericAA;
extern const Font_t UIFont_MediumNumericAA;
extern const Font_t UIFont_SmallNumericAA;
extern const Font_t UIFont_LabelAA;

uint16_t UIFont_BlendRGB565(uint16_t foreground, uint16_t background, uint8_t alpha);
const FontGlyph_t *UIFont_FindGlyph(const Font_t *font, char ch);
uint8_t UIFont_FontCanDraw(const Font_t *font, const char *text);
uint16_t UIFont_GetStringWidthAA(const Font_t *font, const char *text);
uint16_t UIFont_GetStringHeightAA(const Font_t *font);
void UIFont_DrawGlyphAA(uint16_t x, uint16_t y, const Font_t *font, const FontGlyph_t *glyph, uint16_t foreground, uint16_t background);
void UIFont_DrawStringAA(uint16_t x, uint16_t y, const Font_t *font, const char *text, uint16_t foreground, uint16_t background);

uint8_t UIFont_IsNumericText(const char *text);
uint16_t UIFont_GetNumericTextWidth(const char *text, uint8_t scale);
uint16_t UIFont_GetNumericTextHeight(uint8_t scale);
void UIFont_DrawNumericString(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t background, uint8_t scale);
void UIFont_DrawBoldString(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t background, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif /* __UI_FONT_H */

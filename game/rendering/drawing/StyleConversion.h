
#pragma once

/**
 * Contains conversion functions for Style enums to their DirectWrite equivalents.
 */

#include <dwrite.h>
#include <d2d1_1.h>
#include "Style.h"

inline D2D_COLOR_F ConvertColor(uint32_t packedColor) {
  D2D_COLOR_F color;
  color.b = static_cast<float>(packedColor & 0xFF) / 256.0f;
  color.g = static_cast<float>((packedColor >> 8) & 0xFF) / 256.0f;
  color.r = static_cast<float>((packedColor >> 16) & 0xFF) / 256.0f;
  color.a = static_cast<float>((packedColor >> 24) & 0xFF) / 256.0f;
  return color;
}

inline DWRITE_FONT_WEIGHT ConvertFontWeight(FontWeight fontWeight) {
  return static_cast<DWRITE_FONT_WEIGHT>(fontWeight);
}

inline DWRITE_FONT_STRETCH ConvertFontStretch(FontStretch fontStretch) {
  switch (fontStretch) {
    case FontStretch::UltraCondensed:
      return DWRITE_FONT_STRETCH_ULTRA_CONDENSED;
    case FontStretch::ExtraCondensed:
      return DWRITE_FONT_STRETCH_EXTRA_CONDENSED;
    case FontStretch::Condensed:
      return DWRITE_FONT_STRETCH_CONDENSED;
    case FontStretch::SemiCondensed:
      return DWRITE_FONT_STRETCH_SEMI_CONDENSED;
    default:
    case FontStretch::Normal:
      return DWRITE_FONT_STRETCH_NORMAL;
    case FontStretch::SemiExpanded:
      return DWRITE_FONT_STRETCH_SEMI_EXPANDED;
    case FontStretch::Expanded:
      return DWRITE_FONT_STRETCH_EXPANDED;
    case FontStretch::ExtraExpanded:
      return DWRITE_FONT_STRETCH_EXTRA_EXPANDED;
    case FontStretch::UltraExpanded:
      return DWRITE_FONT_STRETCH_ULTRA_EXPANDED;
  }
}

inline DWRITE_FONT_STYLE ConvertFontStyle(FontStyle fontStyle) {
  switch (fontStyle) {
    default:
    case FontStyle::Normal:
      return DWRITE_FONT_STYLE_NORMAL;
    case FontStyle::Italic:
      return DWRITE_FONT_STYLE_ITALIC;
    case FontStyle::Oblique:
      return DWRITE_FONT_STYLE_OBLIQUE;
  }
}

inline DWRITE_TEXT_ALIGNMENT ConvertTextAlign(TextAlign textAlign) {
  switch (textAlign) {
    default:
    case TextAlign::Left:
      return DWRITE_TEXT_ALIGNMENT_LEADING;
    case TextAlign::Center:
      return DWRITE_TEXT_ALIGNMENT_CENTER;
    case TextAlign::Right:
      return DWRITE_TEXT_ALIGNMENT_TRAILING;
    case TextAlign::Justified:
      return DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
  }
}

inline DWRITE_PARAGRAPH_ALIGNMENT ConvertParagraphAlignment(ParagraphAlign paragraphAlign) {
  switch (paragraphAlign) {
    default:
    case ParagraphAlign::Near:
      return DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    case ParagraphAlign::Far:
      return DWRITE_PARAGRAPH_ALIGNMENT_FAR;
    case ParagraphAlign::Center:
      return DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
  }
}

inline DWRITE_WORD_WRAPPING ConvertWordWrap(WordWrap wordWrap) {
  switch (wordWrap) {
    default:
    case WordWrap::Wrap:
      return DWRITE_WORD_WRAPPING_WRAP;
    case WordWrap::NoWrap:
      return DWRITE_WORD_WRAPPING_NO_WRAP;
    case WordWrap::EmergencyBreak:
      return DWRITE_WORD_WRAPPING_EMERGENCY_BREAK;
    case WordWrap::WholeWord:
      return DWRITE_WORD_WRAPPING_WHOLE_WORD;
    case WordWrap::Character:
      return DWRITE_WORD_WRAPPING_CHARACTER;
  }
}

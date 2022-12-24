
#pragma once

#include <winrt/base.h>
#include <algorithm>

enum class TextAlign : int { Left = 0, Center, Right, Justified };

enum class ParagraphAlign : int { Near = 0, Far, Center };

/**
 * Look here for a good explanation of what can be set and how it works.
 * https://learn.microsoft.com/en-us/windows/win32/api/dwrite_3/ns-dwrite_3-dwrite_line_spacing
 */
enum class LineSpacingMode : int { Default = 0, Uniform, Proportional };

enum class FontStretch : int {
  UltraCondensed = 0,
  ExtraCondensed,
  Condensed,
  SemiCondensed,
  Normal,
  SemiExpanded,
  Expanded,
  ExtraExpanded,
  UltraExpanded
};

enum class FontStyle : int { Normal = 0, Italic, Oblique };

// See https://developer.mozilla.org/en-US/docs/Web/CSS/font-weight#common_weight_name_mapping
// Maps directly to the DirectWrite weight enum
enum class FontWeight : int {
  Thin = 100,
  ExtraLight = 200,
  Light = 300,
  Regular = 400,
  Medium = 500,
  SemiBold = 600,
  Bold = 700,
  ExtraBold = 800,
  Black = 900,
  ExtraBlack = 950
};

enum class WordWrap : int {
  Wrap = 0,
  NoWrap,
  EmergencyBreak,
  WholeWord,
  Character
};

/**
 * Defines how text that overflows the layout box is handled.
 */
enum class TrimMode {
  /**
   * No trimming occurs, the text simply overflows.
   */
  None = 0,
  /**
   * Text can be trimmed mid-word at the character level.
   */
  Character = 1,
  /**
   * Text can only be trimmed at the word-level.
   */
  Word = 2,
};

/**
 * Controls how trimming is indicated to the user. See TrimMode.
 */
enum class TrimmingSign {
  /**
   * When trimming occurs, text is simply clipped.
   */
  None = 0,
  /**
   * When trimming occurs, as ellipsis is inserted.
   */
  Ellipsis,
};

struct ParagraphStyle {
  bool HangingIndent;
  float Indent;
  float TabStopWidth;
  TextAlign TextAlignment;
  ParagraphAlign ParagraphAlignment;
  WordWrap WordWrap;
  TrimMode TrimMode;
  TrimmingSign TrimmingSign;
  LineSpacingMode LineSpacingMode; // Default, Uniform, Proportional
  // If mode is proportional, this is a factor
  // If mode is default this is ignored
  // If mode is uniform, this is an absolute pixel value
  float LineHeight;
};

struct TextStyle {
  const wchar_t *FontFace;
  float FontSize;
  // ARGB (high->low)
  uint32_t Color;
  bool Underline;
  bool LineThrough;
  bool Kerning;
  FontStretch FontStretch;
  FontStyle FontStyle;
  FontWeight FontWeight;
  uint32_t DropShadowColor;
  uint32_t OutlineColor;
  float OutlineWidth;
};

/**
 * Bit-Field used to indicate which properties of a given TextStyle structure should
 * actually be applied.
 */
enum class TextStyleProperty : int {
  FontFace = 1 << 0,
  FontSize = 1 << 1,
  Color = 1 << 2,
  Underline = 1 << 3,
  LineThrough = 1 << 4,
  FontStretch = 1 << 5,
  FontStyle = 1 << 6,
  FontWeight = 1 << 7,
  DropShadowColor = 1 << 8,
  Outline = 1 << 9,
  Kerning = 1 << 10,
};

inline constexpr TextStyleProperty operator|(TextStyleProperty a, TextStyleProperty b)
{
  return static_cast<TextStyleProperty>(static_cast<int>(a) | static_cast<int>(b));
}
inline constexpr TextStyleProperty operator&(TextStyleProperty a, TextStyleProperty b)
{
  return static_cast<TextStyleProperty>(static_cast<int>(a) & static_cast<int>(b));
}

inline bool IsTransparent(uint32_t color) {
  return (color & 0xFF000000) == 0;
}

struct BackgroundAndBorderStyle {
  float radiusX = 0;
  float radiusY = 0;
  uint32_t backgroundColor = 0;
  uint32_t borderColor = 0;
  float borderWidth = 0;
};

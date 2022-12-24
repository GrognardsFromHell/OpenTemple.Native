
#pragma once

#include <algorithm>
#include <string>

#include "Style.h"

/**
 * Uniquely identifies a DirectWrite textformat for caching purposes.
 */
struct TextFormatKey {
  TextFormatKey(const ParagraphStyle &paragraphStyle, const TextStyle &textStyle)
      : FontFamilyName(textStyle.FontFace),
        FontWeight(textStyle.FontWeight),
        FontStyle(textStyle.FontStyle),
        FontStretch(textStyle.FontStretch),
        FontSize(textStyle.FontSize),
        TabStopWidth(paragraphStyle.TabStopWidth),
        TextAlignment(paragraphStyle.TextAlignment),
        ParagraphAlignment(paragraphStyle.ParagraphAlignment),
        WordWrap(paragraphStyle.WordWrap),
        TrimMode(paragraphStyle.TrimMode),
        TrimmingSign(paragraphStyle.TrimmingSign),
        LineSpacingMode(paragraphStyle.LineSpacingMode),
        LineHeight(paragraphStyle.LineHeight) {}

  const std::wstring FontFamilyName;
  const FontWeight FontWeight;
  const FontStyle FontStyle;
  const FontStretch FontStretch;
  const float FontSize;
  const float TabStopWidth;
  const TextAlign TextAlignment;
  const ParagraphAlign ParagraphAlignment;
  const WordWrap WordWrap;
  const TrimMode TrimMode;
  const TrimmingSign TrimmingSign;
  const LineSpacingMode LineSpacingMode;
  const float LineHeight;
};

namespace std {
template <>
struct hash<TextFormatKey> {
  size_t operator()(const TextFormatKey &key) const noexcept {
    return std::hash<wstring>()(key.FontFamilyName) ^       //
           std::hash<int>()((int)key.FontWeight) ^          //
           std::hash<int>()((int)key.FontStyle) ^           //
           std::hash<int>()((int)key.FontStretch) ^         //
           std::hash<float>()(key.FontSize) ^               //
           std::hash<float>()(key.TabStopWidth) ^           //
           std::hash<int>()((int)key.TextAlignment) ^       //
           std::hash<int>()((int)key.ParagraphAlignment) ^  //
           std::hash<int>()((int)key.WordWrap) ^            //
           std::hash<int>()((int)key.TrimMode) ^            //
           std::hash<int>()((int)key.TrimmingSign) ^        //
           std::hash<int>()((int)key.LineSpacingMode) ^     //
           std::hash<float>()(key.LineHeight);
  }
};

template <>
struct equal_to<TextFormatKey> {
  bool operator()(const TextFormatKey &left, const TextFormatKey &right) const noexcept {
    return left.FontFamilyName == right.FontFamilyName &&          //
           left.FontWeight == right.FontWeight &&                  //
           left.FontStyle == right.FontStyle &&                    //
           left.FontStretch == right.FontStretch &&                //
           left.FontSize == right.FontSize &&                      //
           left.TabStopWidth == right.TabStopWidth &&              //
           left.TextAlignment == right.TextAlignment &&            //
           left.ParagraphAlignment == right.ParagraphAlignment &&  //
           left.WordWrap == right.WordWrap &&                      //
           left.TrimMode == right.TrimMode &&                      //
           left.TrimmingSign == right.TrimmingSign &&                      //
           left.LineSpacingMode == right.LineSpacingMode &&        //
           left.LineHeight == right.LineHeight;
  }
};
}  // namespace std

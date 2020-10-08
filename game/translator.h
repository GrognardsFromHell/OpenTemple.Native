
#pragma once

#include <QTranslator>

class DynamicTranslator : public QTranslator {
 public:
  explicit DynamicTranslator(
      std::function<const char16_t *(const char *, const char *, const char *, int)> translator);

  bool isEmpty() const override;

  QString translate(const char *context, const char *sourceText, const char *disambiguation,
                    int n) const override;

 private:
  const std::function<const char16_t *(const char *, const char *, const char *, int)> _translator;
};

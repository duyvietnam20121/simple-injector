#pragma once

#include <QJsonValue>
#include <QString>

namespace SimpleInjector {

/// Converts a JSON value (string / bool / number) to an editable string.
/// Sets `*ok = false` for unsupported JSON types (null, array, object).
QString jsonValueToString(const QJsonValue &value, bool *ok = nullptr);

/// Parses "true"/"false", decimal numbers and hexadecimal "0x..." numbers. Only values that
/// fit a signed 32-bit integer are accepted.
bool parseInt32Value(const QString &text, qint32 *out);

} // namespace SimpleInjector

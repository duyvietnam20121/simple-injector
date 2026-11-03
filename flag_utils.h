#pragma once

#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <cstdint>

namespace EzFlag {

// Removes one recognized Roblox flag type prefix (FFlag, DFInt, FString, etc.).
QString cleanPrefix(QString name);

// Converts JSON string, boolean, or number values to their editable text form.
// Sets *ok to false for unsupported JSON types when the pointer is provided.
QString jsonValueToString(const QJsonValue &value, bool *ok = nullptr);

// Parses booleans, decimal integers, and 0x-prefixed hexadecimal integers.
// Only values in the signed 32-bit range are accepted.
bool parseInt32Value(const QString &text, qint32 *out);

// Extracts `Name = 0x...` entries from an offsets header.
QHash<QString, quintptr> parseOffsetsHeader(const QString &source);

// Reads/writes JSON objects. Writes use QSaveFile for atomic replacement.
bool writeJsonAtomically(const QString &path, const QJsonObject &object, QString *error = nullptr);
QJsonObject readJsonObject(const QString &path, const QJsonObject &fallback = {});

} // namespace EzFlag

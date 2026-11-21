#pragma once

#include <QJsonObject>
#include <QString>

namespace SimpleInjector {

/// Parses JSON text whose root must be an object. Returns an empty object and fills
/// `*error` (if given) when the text is malformed or the root is not an object.
QJsonObject parseJsonObject(const QString &text, QString *error = nullptr);

/// Reads a JSON object from a file. Returns `fallback` if the file is missing,
/// unreadable, malformed, or does not contain an object.
QJsonObject readJsonObject(const QString &path, const QJsonObject &fallback = {});

/// Writes a JSON object to a file atomically (QSaveFile), so a crash or a full disk
/// can never leave a half-written file behind.
bool writeJsonAtomically(const QString &path, const QJsonObject &object, QString *error = nullptr);

} // namespace SimpleInjector

#pragma once

#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <cstdint>

namespace SimpleInjector {

/// Gỡ MỘT tiền tố kiểu flag của Roblox (FFlag, DFInt, FString, SFFlag, ...).
///
/// Tiền tố chỉ bị gỡ khi ký tự ngay sau nó KHÔNG phải chữ thường, nhờ vậy
/// tên như "FIntensity" không bị cắt nhầm thành "ensity".
QString cleanPrefix(QString name);

/// Chuyển giá trị JSON (string / bool / number) sang dạng văn bản có thể sửa.
/// Đặt *ok = false nếu kiểu JSON không được hỗ trợ (null, array, object).
QString jsonValueToString(const QJsonValue &value, bool *ok = nullptr);

/// Phân tích bool ("true"/"false"), số thập phân và số hex "0x...".
/// Chỉ chấp nhận giá trị nằm trong miền số nguyên 32-bit có dấu.
bool parseInt32Value(const QString &text, qint32 *out);

/// Trích các mục `Tên = 0x...` từ header offsets. Bỏ qua offset bằng 0
/// hoặc không biểu diễn được bằng quintptr.
QHash<QString, quintptr> parseOffsetsHeader(const QString &source);

/// Phân tích văn bản JSON có gốc là object. Trả về object rỗng và điền
/// *error (nếu có) khi văn bản sai cú pháp hoặc gốc không phải object.
QJsonObject parseJsonObject(const QString &text, QString *error = nullptr);

/// Ghi/đọc object JSON. Ghi dùng QSaveFile nên thay thế file một cách nguyên tử.
bool writeJsonAtomically(const QString &path, const QJsonObject &object, QString *error = nullptr);
QJsonObject readJsonObject(const QString &path, const QJsonObject &fallback = {});

} // namespace SimpleInjector

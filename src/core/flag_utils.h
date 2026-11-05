#pragma once

#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <cstdint>
#include <optional>

namespace SimpleInjector {

/// Gỡ MỘT tiền tố kiểu flag của Roblox (FFlag, DFInt, FString, SFFlag, ...).
///
/// Không áp luật nào về ký tự đứng sau tiền tố: nhiều flag thật có tên bắt đầu bằng chữ thường
/// (ví dụ `FIntkCageHoleNum`, `DFIntjoinGame...`) nên phải gỡ được. Trường hợp tên vốn có dạng
/// tiền tố nhưng không phải tiền tố được xử lý ở khâu tra cứu (xem lookupOffset), không ở đây.
QString cleanPrefix(QString name);

/// Tra offset của một flag trong bảng offsets. Thử theo thứ tự:
///   1. đúng nguyên tên (bảng offsets thường lưu tên KHÔNG có tiền tố kiểu, nhưng vài tên thật
///      tình cờ bắt đầu bằng "FFlag..."/"FInt..." và phải khớp nguyên văn),
///   2. tên đã gỡ một tiền tố kiểu.
/// Trả về std::nullopt nếu không có.
std::optional<quintptr> lookupOffset(const QHash<QString, quintptr> &offsets, const QString &flagName);

/// Chuyển giá trị JSON (string / bool / number) sang dạng văn bản có thể sửa.
/// Đặt *ok = false nếu kiểu JSON không được hỗ trợ (null, array, object).
QString jsonValueToString(const QJsonValue &value, bool *ok = nullptr);

/// Phân tích bool ("true"/"false"), số thập phân và số hex "0x...".
/// Chỉ chấp nhận giá trị nằm trong miền số nguyên 32-bit có dấu.
bool parseInt32Value(const QString &text, qint32 *out);

/// Trích các mục `Tên = 0x...` từ header offsets. Bỏ qua offset bằng 0
/// hoặc không biểu diễn được bằng quintptr. Mỗi tên được lưu NGUYÊN VĂN; nếu tên có dạng
/// tiền tố + phần còn lại thì lưu thêm bản đã gỡ tiền tố (không ghi đè tên nguyên văn khác),
/// để cả `FIntFoo` lẫn `Foo` đều tra ra.
QHash<QString, quintptr> parseOffsetsHeader(const QString &source);

/// Phân tích văn bản JSON có gốc là object. Trả về object rỗng và điền
/// *error (nếu có) khi văn bản sai cú pháp hoặc gốc không phải object.
QJsonObject parseJsonObject(const QString &text, QString *error = nullptr);

/// Ghi/đọc object JSON. Ghi dùng QSaveFile nên thay thế file một cách nguyên tử.
bool writeJsonAtomically(const QString &path, const QJsonObject &object, QString *error = nullptr);
QJsonObject readJsonObject(const QString &path, const QJsonObject &fallback = {});

} // namespace SimpleInjector

#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace SimpleInjector {

/// Kho lưu các flag người dùng đã thêm (tên đã bỏ tiền tố -> giá trị dạng chuỗi).
///
/// Lớp này chỉ phụ thuộc Qt Core nên kiểm thử được mà không cần giao diện.
class FlagStore {
public:
    struct MergeResult {
        int added = 0;   ///< Số mục được thêm hoặc cập nhật.
        int skipped = 0; ///< Số mục bị bỏ qua (tên rỗng hoặc kiểu giá trị không hỗ trợ).
    };

    explicit FlagStore(QString path = {});

    void setPath(QString path) { path_ = std::move(path); }
    const QString &path() const { return path_; }

    /// Nạp từ đĩa; file thiếu hoặc hỏng cho ra kho rỗng.
    void load();
    bool save(QString *error = nullptr) const;

    /// Gộp object JSON bên ngoài vào kho (ghi đè mục trùng tên).
    MergeResult merge(const QJsonObject &incoming);

    /// Đặt giá trị cho một mục ĐÃ TỒN TẠI; false nếu tên không có trong kho.
    bool setValue(const QString &name, const QString &value);

    /// Xóa các mục theo tên, trả về số mục thực sự bị xóa.
    int remove(const QStringList &names);

    /// Trích tập con theo tên (bỏ qua tên không tồn tại).
    QJsonObject subset(const QStringList &names) const;

    const QJsonObject &entries() const { return entries_; }
    int size() const { return entries_.size(); }
    bool isEmpty() const { return entries_.isEmpty(); }

private:
    QString path_;
    QJsonObject entries_;
};

} // namespace SimpleInjector

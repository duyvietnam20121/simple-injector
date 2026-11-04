#pragma once

#include "core/offsets_cache.h"

#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

namespace SimpleInjector {

/// Quản lý bảng offsets: nạp cache cục bộ, tải bản mới bất đồng bộ, rồi ghi lại cache.
class OffsetsService final : public QObject {
    Q_OBJECT

public:
    enum class Outcome {
        Updated,      ///< Tải và phân tích thành công, cache đã cập nhật.
        NetworkError, ///< Lỗi mạng / HTTP; giữ nguyên offsets hiện có.
        EmptyResult,  ///< Tải được nhưng không có mục hợp lệ; giữ nguyên offsets hiện có.
        TooLarge,     ///< Phản hồi vượt giới hạn kích thước; bị từ chối.
    };

    OffsetsService(QUrl sourceUrl, QString cachePath, QObject *parent = nullptr);

    /// Nạp cache từ đĩa (gọi một lần khi khởi động).
    void loadCache();

    /// Bắt đầu tải; bỏ qua nếu đang có yêu cầu chạy.
    void refresh();

    const OffsetMap &offsets() const { return offsets_; }
    bool isEmpty() const { return offsets_.isEmpty(); }
    bool isRefreshing() const { return inFlight_; }

signals:
    void refreshStarted();
    /// `detail` chứa mô tả lỗi khi outcome khác Updated.
    void refreshFinished(SimpleInjector::OffsetsService::Outcome outcome, const QString &detail);

private:
    void onReplyFinished(QNetworkReply *reply);

    static constexpr qint64 kMaxBodyBytes = 8 * 1024 * 1024;
    static constexpr int kTimeoutMs = 10000;

    QUrl sourceUrl_;
    QString cachePath_;
    OffsetMap offsets_;
    QNetworkAccessManager *network_ = nullptr;
    bool inFlight_ = false;
};

} // namespace SimpleInjector

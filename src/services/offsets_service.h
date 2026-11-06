#pragma once

#include "core/offsets_cache.h"

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

namespace SimpleInjector {

/// Quản lý bảng offsets: nạp cache cục bộ, tải bản mới bất đồng bộ (thủ công hoặc
/// định kỳ), nhận diện phiên bản Roblox mà bảng được tạo ra, rồi ghi lại cache.
class OffsetsService final : public QObject {
    Q_OBJECT

public:
    enum class Outcome {
        Updated,      ///< Tải và phân tích thành công, cache đã cập nhật.
        NetworkError, ///< Lỗi mạng / HTTP / hết thời gian; giữ nguyên offsets hiện có.
        EmptyResult,  ///< Tải được nhưng không có mục hợp lệ; giữ nguyên offsets hiện có.
        TooLarge,     ///< Phản hồi vượt giới hạn kích thước; bị từ chối.
    };
    Q_ENUM(Outcome)

    /// Bảng offsets hiện tại đến từ đâu: giúp người dùng biết mình đang dùng dữ liệu mới hay cũ.
    enum class Source {
        None,    ///< Chưa có dữ liệu.
        Cache,   ///< Nạp từ file cache trên đĩa (có thể đã cũ).
        Network, ///< Vừa tải từ nguồn trực tuyến.
    };
    Q_ENUM(Source)

    OffsetsService(QUrl sourceUrl, QString cachePath, QObject *parent = nullptr);

    /// Nạp cache (offsets + phiên bản) từ đĩa; gọi một lần khi khởi động.
    void loadCache();

    /// Bắt đầu tải; bỏ qua nếu đang có yêu cầu chạy.
    void refresh();

    /// Tải lại mỗi `intervalMs` mili giây. Gọi lại để đổi chu kỳ; `stopAutoRefresh()` để dừng.
    void startAutoRefresh(int intervalMs);
    void stopAutoRefresh();
    bool isAutoRefreshing() const { return autoTimer_.isActive(); }

    void setTimeoutMs(int ms) { timeoutMs_ = ms; }
    void setMaxBodyBytes(qint64 bytes) { maxBodyBytes_ = bytes; }

    const OffsetMap &offsets() const { return offsets_; }
    /// Phiên bản Roblox mà offsets hiện tại thuộc về (rỗng nếu không biết).
    const QString &version() const { return version_; }
    Source source() const { return source_; }
    /// Thời điểm dữ liệu hiện tại được tải (mạng) hoặc được lưu (cache); không hợp lệ nếu chưa có.
    const QDateTime &loadedAt() const { return loadedAt_; }
    /// Lỗi của lần tải gần nhất; rỗng nếu lần gần nhất thành công (hoặc chưa tải lần nào).
    const QString &lastError() const { return lastError_; }
    bool isEmpty() const { return offsets_.isEmpty(); }
    bool isRefreshing() const { return inFlight_; }

signals:
    void refreshStarted();
    /// `detail` chứa mô tả lỗi khi outcome khác Updated.
    void refreshFinished(SimpleInjector::OffsetsService::Outcome outcome, const QString &detail);

private:
    void onReplyFinished(QNetworkReply *reply);

    QUrl sourceUrl_;
    QString cachePath_;
    OffsetMap offsets_;
    QString version_;
    Source source_ = Source::None;
    QDateTime loadedAt_;
    QString lastError_;
    QNetworkAccessManager *network_ = nullptr;
    QTimer autoTimer_;
    qint64 maxBodyBytes_ = 8 * 1024 * 1024;
    int timeoutMs_ = 10000;
    bool inFlight_ = false;
};

} // namespace SimpleInjector

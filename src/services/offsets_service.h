#pragma once

#include "core/offsets/offset_map.h"

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

namespace SimpleInjector {

/// Owns the offsets table: loads the local cache, downloads a fresh copy asynchronously
/// (manually or periodically), detects which Roblox version the table was generated for,
/// and writes the cache back.
class OffsetsService final : public QObject {
    Q_OBJECT

public:
    enum class Outcome {
        Updated,      ///< Downloaded and parsed successfully; the cache was updated.
        NetworkError, ///< Network / HTTP error or timeout; the existing offsets are kept.
        EmptyResult,  ///< Downloaded, but no valid entries; the existing offsets are kept.
        TooLarge,     ///< The response exceeded the size limit and was rejected.
    };
    Q_ENUM(Outcome)

    /// Where the current table came from, so users can tell fresh data from stale data.
    enum class Source {
        None,    ///< No data yet.
        Cache,   ///< Loaded from the cache file on disk (may be stale).
        Network, ///< Just downloaded from the online source.
    };
    Q_ENUM(Source)

    OffsetsService(QUrl sourceUrl, QString cachePath, QObject *parent = nullptr);

    /// Loads the cache (offsets + version) from disk; call once at startup.
    void loadCache();

    /// Starts a download; ignored while another one is running.
    void refresh();

    /// Refreshes every `intervalMs` milliseconds. Call again to change the interval;
    /// `stopAutoRefresh()` stops it.
    void startAutoRefresh(int intervalMs);
    void stopAutoRefresh();
    bool isAutoRefreshing() const { return autoTimer_.isActive(); }

    void setTimeoutMs(int ms) { timeoutMs_ = ms; }
    void setMaxBodyBytes(qint64 bytes) { maxBodyBytes_ = bytes; }

    const OffsetMap &offsets() const { return offsets_; }
    /// Roblox version the current offsets belong to (empty if unknown).
    const QString &version() const { return version_; }
    Source source() const { return source_; }
    /// When the current data was downloaded (network) or saved (cache); invalid if there is none.
    const QDateTime &loadedAt() const { return loadedAt_; }
    /// Error of the most recent download; empty if it succeeded (or none has run).
    const QString &lastError() const { return lastError_; }
    bool isEmpty() const { return offsets_.isEmpty(); }
    bool isRefreshing() const { return inFlight_; }

signals:
    void refreshStarted();
    /// `detail` describes the error when `outcome` is not Updated.
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

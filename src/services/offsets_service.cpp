#include "offsets_service.h"

#include "core/flag_utils.h"
#include "core/roblox_version.h"

#include <QDebug>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace SimpleInjector {

OffsetsService::OffsetsService(QUrl sourceUrl, QString cachePath, QObject *parent)
    : QObject(parent)
    , sourceUrl_(std::move(sourceUrl))
    , cachePath_(std::move(cachePath))
    , network_(new QNetworkAccessManager(this))
{
    connect(network_, &QNetworkAccessManager::finished, this, &OffsetsService::onReplyFinished);
    connect(&autoTimer_, &QTimer::timeout, this, &OffsetsService::refresh);
}

void OffsetsService::loadCache()
{
    offsets_ = loadOffsetsCache(cachePath_, &version_);
    source_ = offsets_.isEmpty() ? Source::None : Source::Cache;
    loadedAt_ = offsets_.isEmpty() ? QDateTime() : QFileInfo(cachePath_).lastModified();
}

void OffsetsService::startAutoRefresh(int intervalMs)
{
    autoTimer_.start(intervalMs);
}

void OffsetsService::stopAutoRefresh()
{
    autoTimer_.stop();
}

void OffsetsService::refresh()
{
    if (inFlight_)
        return;
    inFlight_ = true;

    QNetworkRequest request(sourceUrl_);
    request.setTransferTimeout(timeoutMs_);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    network_->get(request);
    emit refreshStarted();
}

void OffsetsService::onReplyFinished(QNetworkReply *reply)
{
    inFlight_ = false;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        lastError_ = reply->errorString();
        emit refreshFinished(Outcome::NetworkError, lastError_);
        return;
    }

    const QByteArray body = reply->readAll();
    if (body.size() > maxBodyBytes_) {
        lastError_ = QStringLiteral("Phản hồi lớn %1 byte, vượt giới hạn %2")
                         .arg(body.size()).arg(maxBodyBytes_);
        emit refreshFinished(Outcome::TooLarge, lastError_);
        return;
    }

    const QString source = QString::fromUtf8(body);
    OffsetMap parsed = parseOffsetsHeader(source);
    if (parsed.isEmpty()) {
        lastError_ = QStringLiteral("Nguồn không có mục offset hợp lệ");
        emit refreshFinished(Outcome::EmptyResult, lastError_);
        return;
    }

    offsets_ = std::move(parsed);
    version_ = parseOffsetsVersion(source);
    source_ = Source::Network;
    loadedAt_ = QDateTime::currentDateTime();
    lastError_.clear();

    QString saveError;
    if (!saveOffsetsCache(cachePath_, offsets_, version_, &saveError))
        qWarning("Không lưu được cache offsets: %s", qPrintable(saveError));

    emit refreshFinished(Outcome::Updated, {});
}

} // namespace SimpleInjector

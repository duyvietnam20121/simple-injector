#include "services/offsets_service.h"

#include "core/offsets/offsets_cache.h"
#include "core/offsets/offsets_header_parser.h"
#include "core/offsets/roblox_version.h"

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
        lastError_ = QStringLiteral("The response is %1 bytes, over the %2-byte limit")
                         .arg(body.size()).arg(maxBodyBytes_);
        emit refreshFinished(Outcome::TooLarge, lastError_);
        return;
    }

    const QString source = QString::fromUtf8(body);
    OffsetMap parsed = parseOffsetsHeader(source);
    if (parsed.isEmpty()) {
        lastError_ = QStringLiteral("The source contains no valid offset entries");
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
        qWarning("Could not save the offsets cache: %s", qPrintable(saveError));

    emit refreshFinished(Outcome::Updated, {});
}

} // namespace SimpleInjector

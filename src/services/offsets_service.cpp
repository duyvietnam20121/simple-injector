#include "offsets_service.h"

#include "core/flag_utils.h"

#include <QDebug>
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
}

void OffsetsService::loadCache()
{
    offsets_ = loadOffsetsCache(cachePath_);
}

void OffsetsService::refresh()
{
    if (inFlight_)
        return;
    inFlight_ = true;

    QNetworkRequest request(sourceUrl_);
    request.setTransferTimeout(kTimeoutMs);
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
        emit refreshFinished(Outcome::NetworkError, reply->errorString());
        return;
    }

    const QByteArray body = reply->readAll();
    if (body.size() > kMaxBodyBytes) {
        emit refreshFinished(Outcome::TooLarge,
                             QStringLiteral("Phản hồi lớn %1 byte, vượt giới hạn").arg(body.size()));
        return;
    }

    OffsetMap parsed = parseOffsetsHeader(QString::fromUtf8(body));
    if (parsed.isEmpty()) {
        emit refreshFinished(Outcome::EmptyResult, QStringLiteral("Nguồn không có mục offset hợp lệ"));
        return;
    }

    offsets_ = std::move(parsed);

    QString saveError;
    if (!saveOffsetsCache(cachePath_, offsets_, &saveError))
        qWarning("Không lưu được cache offsets: %s", qPrintable(saveError));

    emit refreshFinished(Outcome::Updated, {});
}

} // namespace SimpleInjector

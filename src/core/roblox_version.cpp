#include "roblox_version.h"

#include <QRegularExpression>
#include <QStringList>

namespace SimpleInjector {

QString parseOffsetsVersion(const QString &source)
{
    static const QRegularExpression declared(
        QStringLiteral(R"re(ClientVersion\s*=\s*"(version-[0-9A-Za-z]+)")re"));
    const QRegularExpressionMatch declaredMatch = declared.match(source);
    if (declaredMatch.hasMatch())
        return declaredMatch.captured(1);

    static const QRegularExpression loose(QStringLiteral(R"(version-[0-9A-Fa-f]{8,})"));
    const QRegularExpressionMatch looseMatch = loose.match(source.left(2048));
    return looseMatch.hasMatch() ? looseMatch.captured(0) : QString();
}

QString versionFromImagePath(const QString &imagePath)
{
    QString normalized = imagePath;
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    const QStringList parts = normalized.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.size() < 2)
        return {};

    const QString &parent = parts.at(parts.size() - 2);
    return parent.startsWith(QStringLiteral("version-"), Qt::CaseInsensitive) ? parent : QString();
}

VersionMatch compareVersions(const QString &offsetsVersion, const QString &clientVersion)
{
    if (offsetsVersion.isEmpty() || clientVersion.isEmpty())
        return VersionMatch::Unknown;
    return offsetsVersion.compare(clientVersion, Qt::CaseInsensitive) == 0
        ? VersionMatch::Match
        : VersionMatch::Mismatch;
}

} // namespace SimpleInjector

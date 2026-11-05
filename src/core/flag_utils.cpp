#include "flag_utils.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSaveFile>
#include <QPair>
#include <QStringList>
#include <QVector>

#include <limits>

namespace SimpleInjector {

QString cleanPrefix(QString name)
{
    name = name.trimmed();

    // Tiền tố dài hơn phải đứng trước tiền tố là tiền tố con của nó
    // (DFString trước FString, DFFlag trước FFlag, ...).
    static const QStringList prefixes = {
        QStringLiteral("DFString"), QStringLiteral("FString"),
        QStringLiteral("DFInt"),    QStringLiteral("FInt"),
        QStringLiteral("DFFlag"),   QStringLiteral("SFFlag"),
        QStringLiteral("FFlag"),    QStringLiteral("DFLog"),
        QStringLiteral("FLog"),
    };

    for (const QString &prefix : prefixes) {
        if (!name.startsWith(prefix, Qt::CaseSensitive))
            continue;

        name.remove(0, prefix.size());
        break;
    }
    return name.trimmed();
}

std::optional<quintptr> lookupOffset(const QHash<QString, quintptr> &offsets, const QString &flagName)
{
    const QString trimmed = flagName.trimmed();

    const auto exact = offsets.constFind(trimmed);
    if (exact != offsets.cend())
        return exact.value();

    const QString cleaned = cleanPrefix(trimmed);
    if (cleaned != trimmed) {
        const auto stripped = offsets.constFind(cleaned);
        if (stripped != offsets.cend())
            return stripped.value();
    }
    return std::nullopt;
}

QString jsonValueToString(const QJsonValue &value, bool *ok)
{
    if (ok)
        *ok = true;

    if (value.isString())
        return value.toString();
    if (value.isBool())
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (value.isDouble())
        return QString::number(value.toDouble(), 'g', 16);

    if (ok)
        *ok = false;
    return {};
}

bool parseInt32Value(const QString &text, qint32 *out)
{
    if (!out)
        return false;

    const QString normalized = text.trimmed().toLower();
    if (normalized == QStringLiteral("true")) {
        *out = 1;
        return true;
    }
    if (normalized == QStringLiteral("false")) {
        *out = 0;
        return true;
    }

    bool ok = false;
    const qlonglong parsed = normalized.startsWith(QStringLiteral("0x"))
        ? normalized.mid(2).toLongLong(&ok, 16)
        : normalized.toLongLong(&ok, 10);

    if (!ok || parsed < std::numeric_limits<qint32>::min()
        || parsed > std::numeric_limits<qint32>::max()) {
        return false;
    }

    *out = static_cast<qint32>(parsed);
    return true;
}

QHash<QString, quintptr> parseOffsetsHeader(const QString &source)
{
    static const QRegularExpression pattern(
        QStringLiteral(R"((\w+)\s*=\s*(0x[0-9A-Fa-f]+))"));

    QHash<QString, quintptr> parsed;
    QVector<QPair<QString, quintptr>> inOrder;

    auto iterator = pattern.globalMatch(source);
    while (iterator.hasNext()) {
        const QRegularExpressionMatch match = iterator.next();
        bool ok = false;
        const qulonglong value = match.captured(2).mid(2).toULongLong(&ok, 16);
        if (!ok || value == 0 || value > std::numeric_limits<quintptr>::max())
            continue;

        const QString name = match.captured(1);
        parsed.insert(name, static_cast<quintptr>(value)); // tên nguyên văn luôn được ưu tiên
        inOrder.append({name, static_cast<quintptr>(value)});
    }

    // Bản đã gỡ tiền tố: chỉ thêm khi chưa có tên nguyên văn nào trùng.
    for (const auto &entry : std::as_const(inOrder)) {
        const QString cleaned = cleanPrefix(entry.first);
        if (!cleaned.isEmpty() && cleaned != entry.first && !parsed.contains(cleaned))
            parsed.insert(cleaned, entry.second);
    }
    return parsed;
}

QJsonObject parseJsonObject(const QString &text, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        if (error) {
            *error = QStringLiteral("Lỗi JSON tại vị trí %1: %2")
                         .arg(parseError.offset)
                         .arg(parseError.errorString());
        }
        return {};
    }
    if (!document.isObject()) {
        if (error)
            *error = QStringLiteral("Gốc JSON phải là object: { \"TênFlag\": giá_trị }");
        return {};
    }
    return document.object();
}

bool writeJsonAtomically(const QString &path, const QJsonObject &object, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    const QByteArray bytes = QJsonDocument(object).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) {
        if (error)
            *error = file.errorString();
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

QJsonObject readJsonObject(const QString &path, const QJsonObject &fallback)
{
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text))
        return fallback;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return fallback;
    return document.object();
}

} // namespace SimpleInjector

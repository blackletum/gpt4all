#include <gpt4all-backend/main.h>

#include <QCoro/QCoroNetworkReply> // IWYU pragma: keep

#include <QByteArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonValue>

#include <coroutine>
#include <expected>
#include <memory>

using namespace Qt::Literals::StringLiterals;


namespace gpt4all::backend {

auto LLMProvider::getVersion() const -> QCoro::Task<DataOrRespErr<QString>>
{
    QNetworkAccessManager nam;
    std::unique_ptr<QNetworkReply> reply(co_await nam.get(QNetworkRequest(m_baseUrl.resolved(u"/api/version"_s))));
    if (reply->error())
        co_return std::unexpected(reply.get());

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(reply->readAll(), &error);
    if (doc.isNull())
        co_return std::unexpected(error);

    assert(doc.isObject());
    auto obj = doc.object();

    auto version = std::as_const(obj).find("version"_L1);
    assert(version != obj.constEnd());

    assert(version->isString());
    co_return version->toString();
}

} // namespace gpt4all::backend

#include "ollama-client.h"

#include "json-helpers.h"

#include <QCoro/QCoroIODevice> // IWYU pragma: keep
#include <QCoro/QCoroNetworkReply> // IWYU pragma: keep
#include <boost/json.hpp>

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

#include <coroutine>
#include <expected>
#include <memory>

using namespace Qt::Literals::StringLiterals;
using namespace gpt4all::backend::ollama;
namespace json = boost::json;


namespace gpt4all::backend {

template <typename T>
auto OllamaClient::getSimple(const QString &endpoint) -> QCoro::Task<DataOrRespErr<T>>
{
    auto value = co_await getSimpleGeneric(endpoint);
    if (value)
        co_return boost::json::value_to<T>(*value);
    co_return std::unexpected(value.error());
}

template auto OllamaClient::getSimple(const QString &) -> QCoro::Task<DataOrRespErr<VersionResponse>>;
template auto OllamaClient::getSimple(const QString &) -> QCoro::Task<DataOrRespErr<ModelsResponse>>;

auto OllamaClient::getSimpleGeneric(const QString &endpoint) -> QCoro::Task<DataOrRespErr<json::value>>
{
    std::unique_ptr<QNetworkReply> reply(m_nam.get(
        QNetworkRequest(m_baseUrl.resolved(u"/api/%1"_s.arg(endpoint)))
    ));
    if (reply->error())
        co_return std::unexpected(reply.get());

    try {
        json::parser p;
        auto coroReply = qCoro(*reply);
        do {
            auto chunk = co_await coroReply.readAll();
            if (reply->error())
                co_return std::unexpected(reply.get());
            p.write(chunk.data(), chunk.size());
        } while (!reply->atEnd());

        co_return p.release();
    } catch (const std::exception &e) {
        co_return std::unexpected(ResponseError(e, std::current_exception()));
    }
}

} // namespace gpt4all::backend

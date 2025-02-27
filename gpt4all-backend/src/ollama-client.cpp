#include "ollama-client.h"

#include "json-helpers.h"
#include "qt-json-stream.h"

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


static auto processResponse(QNetworkReply &reply) -> QCoro::Task<DataOrRespErr<json::value>>
{
    if (reply.error())
        co_return std::unexpected(&reply);

    try {
        json::parser p;
        auto coroReply = qCoro(reply);
        do {
            auto chunk = co_await coroReply.readAll();
            if (reply.error())
                co_return std::unexpected(&reply);
            p.write(chunk.data(), chunk.size());
        } while (!reply.atEnd());

        co_return p.release();
    } catch (const std::exception &e) {
        co_return std::unexpected(ResponseError(e, std::current_exception()));
    }
}


QNetworkRequest OllamaClient::makeRequest(const QString &path) const
{
    QNetworkRequest req(m_baseUrl.resolved(QUrl(path)));
    req.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    return req;
}

template <typename Resp>
auto OllamaClient::get(const QString &path) -> QCoro::Task<DataOrRespErr<Resp>>
{
    auto value = co_await getJson(path);
    if (value)
        co_return json::value_to<Resp>(*value);
    co_return std::unexpected(value.error());
}

template auto OllamaClient::get(const QString &) -> QCoro::Task<DataOrRespErr<VersionResponse>>;
template auto OllamaClient::get(const QString &) -> QCoro::Task<DataOrRespErr<ModelsResponse>>;

template <typename Resp, typename Req>
auto OllamaClient::post(const QString &path, const Req &req) -> QCoro::Task<DataOrRespErr<Resp>>
{
    auto reqJson = json::value_from(req);
    auto value = co_await postJson(path, reqJson);
    if (value)
        co_return json::value_to<Resp>(*value);
    co_return std::unexpected(value.error());
}

template auto OllamaClient::post(const QString &, const ModelInfoRequest &) -> QCoro::Task<DataOrRespErr<ModelInfo>>;

auto OllamaClient::getJson(const QString &path) -> QCoro::Task<DataOrRespErr<json::value>>
{
    std::unique_ptr<QNetworkReply> reply(m_nam.get(makeRequest(path)));
    co_return co_await processResponse(*reply);
}

auto OllamaClient::postJson(const QString &path, const json::value &req) -> QCoro::Task<DataOrRespErr<json::value>>
{
    JsonStreamDevice reqStream(&req);
    std::unique_ptr<QNetworkReply> reply(
        m_nam.post(makeRequest(path), &reqStream)
    );
    co_return co_await processResponse(*reply);
}


} // namespace gpt4all::backend

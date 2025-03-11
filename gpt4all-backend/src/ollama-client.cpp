#include "ollama-client.h"

#include "json-helpers.h" // IWYU pragma: keep
#include "qt-json-stream.h"
#include "rest.h"

#include <QCoro/QCoroIODevice> // IWYU pragma: keep
#include <QCoro/QCoroNetworkReply> // IWYU pragma: keep

#include <QByteArray>
#include <QNetworkRequest>
#include <QRestReply>
#include <QVariant>
#include <QtAssert>

#include <expected>
#include <memory>

using namespace Qt::Literals::StringLiterals;
using namespace gpt4all::backend::ollama;
namespace json = boost::json;


namespace gpt4all::backend {


ResponseError::ResponseError(const QRestReply *reply)
{
    if (reply->hasError()) {
        m_error = reply->networkReply()->error();
    } else if (!reply->isHttpStatusSuccess()) {
        m_error = BadStatus(reply->httpStatus());
    } else
        Q_UNREACHABLE();

    m_errorString = restErrorString(*reply);
}

ResponseError::ResponseError(const boost::system::system_error &e)
    : m_error(e.code())
    , m_errorString(QString::fromUtf8(e.what()))
{
    Q_ASSERT(e.code());
}

QNetworkRequest OllamaClient::makeRequest(const QString &path) const
{
    QNetworkRequest req(m_baseUrl.resolved(QUrl(path)));
    req.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    return req;
}

auto OllamaClient::processResponse(QNetworkReply &reply) -> QCoro::Task<DataOrRespErr<json::value>>
{
    QRestReply restReply(&reply);
    if (reply.error())
        co_return std::unexpected(&restReply);

    auto coroReply = qCoro(reply);
    for (;;) {
        auto chunk = co_await coroReply.readAll();
        if (!restReply.isSuccess())
            co_return std::unexpected(&restReply);
        if (chunk.isEmpty()) {
            Q_ASSERT(reply.atEnd());
            break;
        }
        m_parser.write(chunk.data(), chunk.size());
    }

    m_parser.finish();
    co_return m_parser.release();
}

template <typename Resp>
auto OllamaClient::get(const QString &path) -> QCoro::Task<DataOrRespErr<Resp>>
{
    // get() should not throw exceptions
    try {
        auto value = co_await getJson(path);
        if (value)
            co_return json::value_to<Resp>(*value);
        co_return std::unexpected(value.error());
    } catch (const boost::system::system_error &e) {
        co_return std::unexpected(e);
    }
}

template auto OllamaClient::get(const QString &) -> QCoro::Task<DataOrRespErr<VersionResponse>>;
template auto OllamaClient::get(const QString &) -> QCoro::Task<DataOrRespErr<ListResponse>>;

template <typename Resp, typename Req>
auto OllamaClient::post(const QString &path, const Req &body) -> QCoro::Task<DataOrRespErr<Resp>>
{
    // post() should not throw exceptions
    try {
        auto reqJson = json::value_from(body);
        auto value = co_await postJson(path, reqJson);
        if (value)
            co_return json::value_to<Resp>(*value);
        co_return std::unexpected(value.error());
    } catch (const boost::system::system_error &e) {
        co_return std::unexpected(e);
    }
}

template auto OllamaClient::post(const QString &, const ShowRequest &) -> QCoro::Task<DataOrRespErr<ShowResponse>>;

auto OllamaClient::getJson(const QString &path) -> QCoro::Task<DataOrRespErr<json::value>>
{
    std::unique_ptr<QNetworkReply> reply(m_nam.get(makeRequest(path)));
    co_return co_await processResponse(*reply);
}

auto OllamaClient::postJson(const QString &path, const json::value &body) -> QCoro::Task<DataOrRespErr<json::value>>
{
    JsonStreamDevice stream(&body);
    auto req = makeRequest(path);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"_ba);
    std::unique_ptr<QNetworkReply> reply(m_nam.post(req, &stream));
    co_return co_await processResponse(*reply);
}


} // namespace gpt4all::backend

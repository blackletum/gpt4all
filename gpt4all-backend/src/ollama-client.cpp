#include "ollama-client.h"

#include "json-helpers.h" // IWYU pragma: keep
#include "qt-json-stream.h"

#include <QCoro/QCoroIODevice> // IWYU pragma: keep
#include <QCoro/QCoroNetworkReply> // IWYU pragma: keep

#include <QByteArray>
#include <QNetworkRequest>
#include <QRestReply>
#include <QVariant>
#include <QtAssert>

#include <coroutine>
#include <expected>
#include <memory>

using namespace Qt::Literals::StringLiterals;
using namespace gpt4all::backend::ollama;
namespace json = boost::json;


namespace gpt4all::backend {


ResponseError::ResponseError(const QRestReply *reply)
{
    auto *nr = reply->networkReply();
    if (reply->hasError()) {
        error       = nr->error();
        errorString = nr->errorString();
    } else if (!reply->isHttpStatusSuccess()) {
        auto code   = reply->httpStatus();
        auto reason = nr->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
        error       = BadStatus(code);
        errorString = u"HTTP %1%2%3 for URL \"%4\""_s.arg(
            QString::number(code),
            reason.isValid() ? u" "_s : QString(),
            reason.toString(),
            nr->request().url().toString()
        );
    } else
        Q_UNREACHABLE();
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

#include "ollama_client.h"

#include "json_helpers.h"

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
namespace json = boost::json;


namespace gpt4all::backend {

auto OllamaClient::getVersion() -> QCoro::Task<DataOrRespErr<VersionResponse>>
{
    std::unique_ptr<QNetworkReply> reply(m_nam.get(QNetworkRequest(m_baseUrl.resolved(u"/api/version"_s))));
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

        co_return json::value_to<VersionResponse>(p.release());
    } catch (const std::exception &e) {
        co_return std::unexpected(ResponseError(e, std::current_exception()));
    }
}

} // namespace gpt4all::backend

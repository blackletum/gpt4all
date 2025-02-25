#include <gpt4all-backend/main.h>

#include <QCoro/QCoroNetworkReply> // IWYU pragma: keep

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <coroutine>
#include <expected>
#include <memory>

using namespace Qt::Literals::StringLiterals;


namespace gpt4all::backend {

auto LLMProvider::getVersion() const -> QCoro::Task<DataOrNetErr<QString>>
{
    QNetworkAccessManager nam;
    std::unique_ptr<QNetworkReply> reply(co_await nam.get(QNetworkRequest(m_baseUrl.resolved(u"/api/version"_s))));
    if (auto err = reply->error())
        co_return std::unexpected(NetErr{ err, reply->errorString() });

    // TODO(jared): parse JSON here instead of just returning the data
    co_return QString::fromUtf8(reply->readAll());
}

} // namespace gpt4all::backend

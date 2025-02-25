#pragma once

#include <QCoro/QCoroTask> // IWYU pragma: keep

#include <QJsonParseError>
#include <QNetworkReply>
#include <QString>
#include <QUrl>

#include <cassert>
#include <expected>
#include <utility>
#include <variant>


namespace gpt4all::backend {

struct ResponseError {
private:
    using ErrorCode = std::variant<
        QNetworkReply::NetworkError,
        QJsonParseError::ParseError
    >;

public:
    ErrorCode error;
    QString   errorString;

    ResponseError(const QNetworkReply *reply)
        : error(reply->error())
        , errorString(reply->errorString())
    {
        assert(reply->error());
    }

    ResponseError(const QJsonParseError &err)
        : error(err.error)
        , errorString(err.errorString())
    {
        assert(err.error);
    }
};

template <typename T>
using DataOrRespErr = std::expected<T, ResponseError>;


class LLMProvider {
public:
    LLMProvider(QUrl baseUrl)
        : m_baseUrl(baseUrl)
        {}

    const QUrl &baseUrl() const { return m_baseUrl; }
    void getBaseUrl(QUrl value) { m_baseUrl = std::move(value); }

    /// Retrieve the Ollama version, e.g. "0.5.1"
    auto getVersion() const -> QCoro::Task<DataOrRespErr<QString>>;

private:
    QUrl m_baseUrl;
};

} // namespace gpt4all::backend

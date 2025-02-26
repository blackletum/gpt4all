#pragma once

#include <QCoro/QCoroTask> // IWYU pragma: keep
#include <boost/describe/class.hpp>

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
        std::exception_ptr
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

    ResponseError(const std::exception &e, std::exception_ptr err)
        : error(std::move(err))
        , errorString(e.what())
    {
        assert(std::get<std::exception_ptr>(error));
    }
};

template <typename T>
using DataOrRespErr = std::expected<T, ResponseError>;

struct VersionResponse { QString version; };
BOOST_DESCRIBE_STRUCT(VersionResponse, (), (version))

class OllamaClient {
public:
    OllamaClient(QUrl baseUrl)
        : m_baseUrl(baseUrl)
        {}

    const QUrl &baseUrl() const { return m_baseUrl; }
    void getBaseUrl(QUrl value) { m_baseUrl = std::move(value); }

    /// Retrieve the Ollama version, e.g. "0.5.1"
    auto getVersion() -> QCoro::Task<DataOrRespErr<VersionResponse>>;

private:
    QUrl                  m_baseUrl;
    QNetworkAccessManager m_nam;
};

} // namespace gpt4all::backend

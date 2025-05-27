#pragma once

#include <QHttpServer>
#include <QHttpServerRequest>

#include <functional>
#include <optional>
#include <utility>
#include <vector>

class QHttpServerResponse;
class QHttpServerRouterRule;
class QString;


namespace gpt4all::ui {


/// @brief QHttpServer wrapper with middleware support.
///
/// This class wraps QHttpServer and provides addBeforeRequestHandler() to add middleware.
class MwHttpServer
{
    using BeforeRequestHandler = std::function<std::optional<QHttpServerResponse>(const QHttpServerRequest &)>;

public:
    explicit MwHttpServer();

    bool bind() { return m_httpServer.bind(m_tcpServer); }

    void addBeforeRequestHandler(BeforeRequestHandler handler)
    { m_beforeRequestHandlers.push_back(std::move(handler)); }

    template <typename Handler>
    void addAfterRequestHandler(
        const typename QtPrivate::ContextTypeForFunctor<Handler>::ContextType *context, Handler &&handler
    ) {
        return m_httpServer.addAfterRequestHandler(context, std::forward<Handler>(handler));
    }

    template <typename... Args>
    QHttpServerRouterRule *route(
        const QString &pathPattern,
        QHttpServerRequest::Methods method,
        std::function<QHttpServerResponse(Args..., const QHttpServerRequest &)> viewHandler
    );

    QTcpServer *tcpServer() { return m_tcpServer; }

private:
    QHttpServer                        m_httpServer;
    QTcpServer                        *m_tcpServer;
    std::vector<BeforeRequestHandler>  m_beforeRequestHandlers;
};


} // namespace gpt4all::ui


#include "mwhttpserver.inl" // IWYU pragma: export

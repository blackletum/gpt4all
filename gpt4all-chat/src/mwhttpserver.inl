namespace gpt4all::ui {


template <typename... Args>
QHttpServerRouterRule *MwHttpServer::route(
    const QString &pathPattern,
    QHttpServerRequest::Methods method,
    std::function<QHttpServerResponse(Args..., const QHttpServerRequest &)> viewHandler
) {
    auto wrapped = [this, vh = std::move(viewHandler)](Args ...args, const QHttpServerRequest &req) {
        for (auto &handler : m_beforeRequestHandlers)
            if (auto resp = handler(req))
                return *std::move(resp);
        return vh(std::forward<Args>(args)..., req);
    };
    return m_httpServer.route(pathPattern, method, std::move(wrapped));
}


} // namespace gpt4all::ui

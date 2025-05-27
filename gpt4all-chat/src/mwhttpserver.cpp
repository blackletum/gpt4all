#include <QTcpServer>

#include "mwhttpserver.h"


namespace gpt4all::ui {


MwHttpServer::MwHttpServer()
    : m_httpServer()
    , m_tcpServer (new QTcpServer(&m_httpServer))
    {}


} // namespace gpt4all::ui

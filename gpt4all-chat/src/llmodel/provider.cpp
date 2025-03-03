#include "provider.h"

#include <utility>


namespace gpt4all::ui {


void OpenaiProvider::setBaseUrl(QUrl value)
{
    if (m_baseUrl != value) {
        m_baseUrl = std::move(value);
        emit baseUrlChanged(m_baseUrl);
    }
}

void OpenaiProvider::setApiKey(QString value)
{
    if (m_apiKey != value) {
        m_apiKey = std::move(value);
        emit apiKeyChanged(m_apiKey);
    }
}


} // namespace gpt4all::ui

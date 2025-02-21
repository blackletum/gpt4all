#include <QCoro/QCoroTask>
#include <QLatin1StringView>

import fmt;
import gpt4all.backend.main;
import gpt4all.test.config;

using gpt4all::backend::LLMProvider;


int main()
{
    LLMProvider provider(OLLAMA_URL);
    auto version = QCoro::waitFor(provider.getVersion());
    if (version) {
        fmt::print("Server version: {}", *version);
    } else {
        fmt::print("Network error: {}", version.unexpected().errorString);
    }
}

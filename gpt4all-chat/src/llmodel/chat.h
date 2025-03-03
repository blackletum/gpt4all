#pragma once

#include <QStringView>

class QString;
namespace QCoro { template <typename T> class AsyncGenerator; }
namespace gpt4all::backend { struct GenerationParams; }


namespace gpt4all::ui {


struct ChatResponseMetadata {
    int nPromptTokens;
    int nResponseTokens;
};

// TODO: implement two of these; one based on Ollama (TBD) and the other based on OpenAI (chatapi.h)
class ChatLLModel {
public:
    virtual ~ChatLLModel() = 0;

    [[nodiscard]]
    virtual QString name() = 0;

    virtual void preload() = 0;
    virtual auto chat(QStringView prompt, const backend::GenerationParams &params,
                      /*out*/ ChatResponseMetadata &metadata) -> QCoro::AsyncGenerator<QString> = 0;
};


} // namespace gpt4all::ui

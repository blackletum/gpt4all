#pragma once

class QString;
class QStringView;
namespace QCoro {
    template <typename T> class AsyncGenerator;
    template <typename T> class Task;
}


namespace gpt4all::ui {


class GenerationParams;
class ModelDescription;

struct ChatResponseMetadata {
    int nPromptTokens;
    int nResponseTokens;
};

// TODO: implement two of these; one based on Ollama (TBD) and the other based on OpenAI (chatapi.h)
class ChatLLMInstance {
public:
    virtual ~ChatLLMInstance() noexcept = 0;

    virtual auto description() const -> const ModelDescription * = 0;
    virtual auto preload() -> QCoro::Task<void> = 0;
    virtual auto generate(QStringView prompt, const GenerationParams *params, /*out*/ ChatResponseMetadata &metadata)
        -> QCoro::AsyncGenerator<QString> = 0;
};


} // namespace gpt4all::ui

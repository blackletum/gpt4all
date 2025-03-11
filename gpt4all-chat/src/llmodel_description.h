#pragma once

#include <QObject>
#include <QVariant>

#include <memory>

class QNetworkAccessManager;


namespace gpt4all::ui {


class ChatLLMInstance;
class ModelProvider;

// TODO: implement shared_from_this guidance for restricted construction
class ModelDescription : public std::enable_shared_from_this<ModelDescription> {
    Q_GADGET
    Q_PROPERTY(const ModelProvider *provider READ provider CONSTANT)
    Q_PROPERTY(QVariant             key      READ key      CONSTANT)

public:
    virtual ~ModelDescription() noexcept = 0;

    // getters
    [[nodiscard]] virtual auto     provider() const -> const ModelProvider * = 0;
    [[nodiscard]] virtual QVariant key     () const = 0;

    /// create an instance to chat with
    [[nodiscard]] auto newInstance(QNetworkAccessManager *nam) const -> std::unique_ptr<ChatLLMInstance>;

    friend bool operator==(const ModelDescription &a, const ModelDescription &b);

protected:
    [[nodiscard]] virtual auto newInstanceImpl(QNetworkAccessManager *nam) const -> ChatLLMInstance * = 0;
};


} // namespace gpt4all::ui

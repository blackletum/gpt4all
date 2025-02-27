#pragma once

#ifdef G4A_BACKEND_IMPL
#   include <boost/describe/class.hpp>
#   include <boost/describe/enum.hpp>
#endif
#include <boost/json.hpp> // IWYU pragma: keep

#include <QString>
#include <QtTypes>

#include <optional>
#include <vector>


namespace gpt4all::backend::ollama {

//
// basic types
//

/// Details about a model.
struct ModelDetails {
    QString              parent_model;       /// The parent of the model.
    QString              format;             /// The format of the model.
    QString              family;             /// The family of the model.
    std::vector<QString> families;           /// The families of the model.
    QString              parameter_size;     /// The size of the model's parameters.
    QString              quantization_level; /// The quantization level of the model.
};
#ifdef G4A_BACKEND_IMPL
BOOST_DESCRIBE_STRUCT(ModelDetails, (), (parent_model, format, family, families, parameter_size, quantization_level))
#endif

/// A model available locally.
struct Model {
    QString      model;       /// The model name.
    QString      modified_at; /// Model modification date.
    quint64      size;        /// Size of the model on disk.
    QString      digest;      /// The model's digest.
    ModelDetails details;     /// The model's details.
};
#ifdef G4A_BACKEND_IMPL
BOOST_DESCRIBE_STRUCT(Model, (), (model, modified_at, size, digest, details))
#endif

enum MessageRole {
    system,
    user,
    assistant,
    tool,
};
#ifdef G4A_BACKEND_IMPL
BOOST_DESCRIBE_ENUM(MessageRole, system, user, assistant, tool)
#endif

struct ToolCallFunction {
    QString             name;      /// The name of the function to be called.
    boost::json::object arguments; /// The arguments to pass to the function.
};
#ifdef G4A_BACKEND_IMPL
BOOST_DESCRIBE_STRUCT(ToolCallFunction, (), (name, arguments))
#endif

struct ToolCall {
    ToolCallFunction function; /// The function the model wants to call.
};
#ifdef G4A_BACKEND_IMPL
BOOST_DESCRIBE_STRUCT(ToolCall, (), (function))
#endif

/// A message in the chat endpoint
struct Message {
    MessageRole           role;       /// The role of the message
    QString               content;    /// The content of the message
    std::vector<QString>  images;     /// (optional) a list of Base64-encoded images to include in the message
    std::vector<ToolCall> tool_calls; /// A list of tool calls the model wants to call.
};
#ifdef G4A_BACKEND_IMPL
BOOST_DESCRIBE_STRUCT(Message, (), (role, content, images, tool_calls))
#endif

//
// request types
//

/// Request class for the show model info endpoint.
struct ModelInfoRequest {
    QString model; /// The model name.
};
#ifdef G4A_BACKEND_IMPL
BOOST_DESCRIBE_STRUCT(ModelInfoRequest, (), (model))
#endif

//
// response types
//

/// The response class for the version endpoint.
struct VersionResponse {
    QString version; /// The version of the Ollama server.
};
#ifdef G4A_BACKEND_IMPL
BOOST_DESCRIBE_STRUCT(VersionResponse, (), (version))
#endif

/// Response class for the list models endpoint.
struct ModelsResponse {
    std::vector<Model> models; /// List of models available locally.
};
#ifdef G4A_BACKEND_IMPL
BOOST_DESCRIBE_STRUCT(ModelsResponse, (), (models))
#endif

/// Details about a model including modelfile, template, parameters, license, and system prompt.
struct ModelInfo {
    std::optional<QString>              license;    /// The model's license.
    std::optional<QString>              modelfile;  /// The modelfile associated with the model.
    std::optional<QString>              parameters; /// The model parameters.
    std::optional<QString>              template_;  /// The prompt template for the model.
    std::optional<QString>              system;     /// The system prompt for the model.
    ModelDetails                        details;
    boost::json::object                 model_info;
    std::optional<std::vector<Message>> messages;   /// The default messages for the model.
};

#ifdef G4A_BACKEND_IMPL
ModelInfo tag_invoke(const boost::json::value_to_tag<ModelInfo> &, const boost::json::value &value);
#endif

} // namespace gpt4all::backend::ollama

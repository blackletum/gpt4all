#pragma once

#include <boost/describe/class.hpp>

#include <QString>
#include <QtTypes>

#include <vector>


namespace gpt4all::backend::ollama {

/// Details about a model.
struct ModelDetails {
    QString              parent_model;       /// The parent of the model.
    QString              format;             /// The format of the model.
    QString              family;             /// The family of the model.
    std::vector<QString> families;           /// The families of the model.
    QString              parameter_size;     /// The size of the model's parameters.
    QString              quantization_level; /// The quantization level of the model.
};
BOOST_DESCRIBE_STRUCT(ModelDetails, (), (parent_model, format, family, families, parameter_size, quantization_level))

/// A model available locally.
struct Model {
    QString      model;       /// The model name.
    QString      modified_at; /// Model modification date.
    quint64      size;        /// Size of the model on disk.
    QString      digest;      /// The model's digest.
    ModelDetails details;     /// The model's details.
};
BOOST_DESCRIBE_STRUCT(Model, (), (model, modified_at, size, digest, details))


/// The response class for the version endpoint.
struct VersionResponse {
    QString version; /// The version of the Ollama server.
};
BOOST_DESCRIBE_STRUCT(VersionResponse, (), (version))

/// Response class for the list models endpoint.
struct ModelsResponse {
    std::vector<Model> models; /// List of models available locally.
};
BOOST_DESCRIBE_STRUCT(ModelsResponse, (), (models))

} // namespace gpt4all::backend::ollama

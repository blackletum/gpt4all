#pragma once

#include <QJsonValue>
#include <QLatin1StringView> // IWYU pragma: keep

#include <initializer_list>
#include <utility> // IWYU pragma: keep

// IWYU pragma: no_forward_declare QJsonValue
class QJsonObject;


// alternative to QJsonObject's initializer_list constructor that accepts Latin-1 strings
QJsonObject makeJsonObject(std::initializer_list<std::pair<QLatin1StringView, QJsonValue>> args);

#include "utils.inl" // IWYU pragma: export

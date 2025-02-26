#include "json-helpers.h"

#include <boost/json.hpp>

#include <QString>


QString tag_invoke(const boost::json::value_to_tag<QString> &, const boost::json::value &value)
{
    auto &s = value.as_string();
    return QString::fromUtf8(s.data(), s.size());
}

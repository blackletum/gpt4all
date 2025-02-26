#pragma once

class QString;
namespace boost::json {
    class value;
    template <typename T> struct value_to_tag;
}


/// Allows JSON strings to be deserialized as QString.
QString tag_invoke(const boost::json::value_to_tag<QString> &, const boost::json::value &value);

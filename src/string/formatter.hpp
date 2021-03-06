/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
 * \section License
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef STRING_FORMATTER_HPP
#define STRING_FORMATTER_HPP

#include <unordered_map>
#include <sstream>

#include "format_flags.hpp"
#include "color.hpp"
#include "melanolib/utils/singleton.hpp"
#include "melanolib/utils/type_utils.hpp"
#include "melanolib/string/encoding.hpp"

namespace string {

using Unicode = melanolib::string::Unicode;
class FormattedString;


/**
 * \brief Dummy class that represents a complete reset of all flags and colors
 */
class ClearFormatting{};

/**
 * \brief Simple ASCII string
 */
using AsciiString = std::string;

/**
 * \brief Abstract formatting visitor (and factory)
 * \note Formatters should register here
 */
class Formatter
{
public:

    /**
     * \brief Internal factory
     *
     * Factory needs a singleton, Formatter is abstract so it needs a different
     * class, but Formatter is the place to go to access the factory.
     *
     * Also note that this doesn't really create objects since they don't need
     * any data, so the objects are created only once.
     */
    class Registry : public melanolib::Singleton<Registry>
    {
    public:

        ~Registry()
        {
            for ( const auto& f : formatters )
                delete f.second;
        }

        /**
         * \brief Get a registered formatter
         */
        Formatter* formatter(const std::string& name);

        /**
         * \brief Register a formatter
         */
        void add_formatter(Formatter* instance);

    private:
        Registry();
        friend ParentSingleton;

        std::unordered_map<std::string, Formatter*> formatters;
        Formatter* default_formatter = nullptr;
    };

    class Context
    {
    public:
        virtual ~Context(){};
    };

    Formatter() = default;
    Formatter(const Formatter&) = delete;
    Formatter(Formatter&&) = delete;
    Formatter& operator=(const Formatter&) = delete;
    Formatter& operator=(Formatter&&) = delete;

    virtual ~Formatter() {}

    virtual std::unique_ptr<Context> context() const
    {
        return {};
    }

    /**
     * \brief Begin encoding a string
     */
    virtual std::string string_begin(Context* context) const
    {
        return {};
    }

    /**
     * \brief End encoding a string
     */
    virtual std::string string_end(Context* context) const
    {
        return {};
    }

    /**
     * \brief Encode a simple ASCII string
     */
    virtual std::string to_string(const AsciiString& s, Context* context) const
    {
        return s;
    }

    /**
     * \brief Encode a single ASCII character
     */
    virtual std::string to_string(char c, Context* context) const
    {
        return to_string(std::string(1, c), context);
    }

    std::string to_string(int c, Context* context) const = delete;

    /**
     * \brief Encode a unicode (non-ASCII) character
     */
    virtual std::string to_string(const Unicode& c, Context* context) const
    {
        return {};
    }

    /**
     * \brief Encode a color code
     * \todo Background colors?
     */
    virtual std::string to_string(const color::Color12& color, Context* context) const
    {
        return {};
    }

    /**
     * \brief Encode format flags
     */
    virtual std::string to_string(FormatFlags flags, Context* context) const
    {
        return {};
    }

    /**
     * \brief Clear all formatting
     */
    virtual std::string to_string(ClearFormatting, Context* context) const
    {
        return {};
    }

    /**
     * \brief Decode a string
     */
    virtual FormattedString decode(const std::string& source) const = 0;

    /**
     * \brief Name of the format
     */
    virtual std::string name() const = 0;

    /**
     * \brief Get a string formatter from name
     */
    static Formatter* formatter(const std::string& name)
    {
        return Registry::instance().formatter(name);
    }

    /**
     * \brief Get the formatter registry singleton
     */
    static Registry& registry() { return Registry::instance(); }
};

/**
 * \brief Plain UTF-8
 */
class FormatterUtf8 : public Formatter
{
public:
    using Formatter::to_string;
    std::string to_string(const Unicode& c, Context* context) const override;
    FormattedString decode(const std::string& source) const override;
    std::string name() const override;
};

/**
 * \brief Plain ASCII
 */
class FormatterAscii : public Formatter
{
public:
    using Formatter::to_string;
    std::string to_string(const Unicode& c, Context* context) const override;
    FormattedString decode(const std::string& source) const override;
    std::string name() const override;
};

/**
 * \brief ANSI-formatted UTF-8 or ASCII
 */
class FormatterAnsi : public Formatter
{
public:
    explicit FormatterAnsi(bool utf8) : utf8(utf8) {}

    using Formatter::to_string;
    std::string to_string(const Unicode& c, Context* context) const override;
    std::string to_string(const color::Color12& color, Context* context) const override;
    std::string to_string(FormatFlags flags, Context* context) const override;
    std::string to_string(ClearFormatting clear, Context* context) const override;

    FormattedString decode(const std::string& source) const override;
    std::string name() const override;

private:
    bool utf8;
};

/**
 * \brief IRC formatter optimized for black backgrounds
 * \todo Might be better to have a generic color filter option in the formatter
 */
class FormatterAnsiBlack : public FormatterAnsi
{
public:
    using FormatterAnsi::FormatterAnsi;
    using FormatterAnsi::to_string;
    std::string to_string(const color::Color12& color, Context* context) const override;
    std::string name() const override;
};

/**
 * \brief Custom string formatting (Utf-8)
 *
 * Style formatting:
 *      * $$            $
 *      * $(-)          clear all formatting
 *      * $(-b)         bold
 *      * $(-u)         underline
 *      * $(1)          red
 *      * $(xf00)       red
 *      * $(red)        red
 *      * $(nocolor)    no color
 */
class FormatterConfig : public FormatterUtf8
{
public:
    explicit FormatterConfig() {}

    using FormatterUtf8::to_string;
    std::string to_string(char input, Context* context) const override;
    std::string to_string(const AsciiString& s, Context* context) const override;
    std::string to_string(const color::Color12& color, Context* context) const override;
    std::string to_string(FormatFlags flags, Context* context) const override;
    std::string to_string(ClearFormatting clear, Context* context) const override;

    FormattedString decode(const std::string& source) const override;
    std::string name() const override;

};

} // namespace string
#endif // STRING_FORMATTER_HPP

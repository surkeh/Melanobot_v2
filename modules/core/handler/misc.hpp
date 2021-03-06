/**
 * \file
 * \brief This file defines handlers which perform miscellaneous tasks
 *
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
 * \license
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
#ifndef HANDLER_MISC
#define HANDLER_MISC

#include "melanobot/handler.hpp"
#include "melanolib/math/random.hpp"
#include "melanolib/string/language.hpp"
#include "melanolib/time/time_string.hpp"
#include "melanobot/melanobot.hpp"

namespace core {

/**
 * \brief Handler showing licensing information
 * \note Must be enabled to comply to the AGPL
 */
class License : public melanobot::SimpleAction
{
public:
    License(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("license", settings, parent)
    {
        sources_url = settings.get("url", settings::global_settings.get("website", ""));
        help = "Shows licensing information";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg, "AGPLv3+ (http://www.gnu.org/licenses/agpl-3.0.html), Sources: "+sources_url);
        return true;
    }

private:
    std::string sources_url;
};

/**
 * \brief Handler showing help on the available handlers
 * \note It is trongly recommended that this is enabled
 */
class Help : public melanobot::SimpleAction
{
public:
    Help(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("help", settings, parent)
    {
        help = "Shows available commands";
        synopsis += " [command|group]";
        help_group = settings.get("help_group", help_group);
        show_inline = settings.get("inline", show_inline);
    }

protected:
    /// \todo Synopsis/help as formatted strings
    bool on_handle(network::Message& msg) override
    {
        PropertyTree props;
        // AKA get the toplevel parent
        get_parent<melanobot::Melanobot>()->populate_properties(
            {"name", "help", "auth", "synopsis", "help_group", "channels"},
            props);
        
        if ( cleanup(msg, props) )
        {
            PropertyTree result;
            restructure(props, &result);

            auto queried = find(result, msg.message);
            if ( queried )
            {
                string::FormattedString synopsis;
                std::string name = queried->get("name", "");

                if ( !name.empty() )
                    synopsis << color::red << name << color::nocolor;

                std::vector<std::string> names;
                gather(*queried, names);
                if ( !names.empty() )
                {
                    std::sort(names.begin(), names.end());
                    for ( auto& name : names )
                        if ( name.find(' ') != std::string::npos )
                            name = '"' + name + '"';

                    if ( !synopsis.empty() )
                        synopsis << ": ";
                    synopsis << melanolib::string::implode(" ", names);
                }

                std::string synopsis_string = queried->get("synopsis", "");
                if ( !synopsis_string.empty() )
                {
                    if ( !synopsis.empty() )
                        synopsis << ": ";
                    synopsis << string::FormatterConfig().decode(synopsis_string);
                }

                std::string help_raw = queried->get("help", "");
                string::FormattedString help;
                if ( !help_raw.empty() )
                    help << color::dark_blue << string::FormatterConfig().decode(help_raw);

                if ( show_inline )
                {
                    synopsis << string::ClearFormatting() << ": " << help;
                    help.clear();
                }

                reply_to(msg, synopsis.copy());

                if ( !help.empty() )
                    reply_to(msg, help.copy());
            }
            else
            {
                reply_to(msg, string::FormattedString() << "Not found: " <<
                    string::FormatFlags::BOLD << msg.message);
            }
        }
        else
        {
            reply_to(msg, string::FormattedString() << "No help items found");
        }

        return true;
    }

private:
    std::string help_group; ///< Only shows help for members of this group
    bool        show_inline = false; ///< if true, prints a single line.

    /**
     * \brief Remove items the user can't perform
     * \return \b false if \c propeties shall not be considered
     */
    bool cleanup(network::Message& msg, PropertyTree& properties ) const
    {
        if ( !msg.source->user_auth(msg.from.local_id, properties.get("auth", "")) )
            return false;
        if ( properties.get("help_group", help_group) != help_group )
            return false;
        auto channels = properties.get_optional<std::string>("channels");
        if ( channels && !msg.source->channel_mask(msg.channels, *channels) )
            return false;

        for ( auto it = properties.begin(); it != properties.end(); )
        {
            if ( !cleanup(msg, it->second) )
                it = properties.erase(it);
            else
                    ++it;
        }
        return true;
    }

    /**
     * \brief removes all internal nodes which don't have a name key
     */
    melanolib::Optional<PropertyTree> restructure ( const PropertyTree& input, PropertyTree* parent ) const
    {
        melanolib::Optional<PropertyTree> ret;

        if ( input.get_optional<std::string>("name") )
        {
            ret = PropertyTree();
            parent = &*ret;
        }

        for ( const auto& p : input )
        {
            if ( !p.second.empty() )
            {
                auto child = restructure(p.second, parent);
                if ( child )
                {
                    auto merge = parent->get_child_optional(p.first);
                    if ( merge )
                        settings::merge(*merge, *child, true);
                    else
                        parent->put_child(p.first, *child);
                }
            }
            else if ( ret && !p.second.data().empty() )
            {
                ret->put(p.first, p.second.data());
            }
        }

        return ret;
    }

    /**
     * \brief Gathers top-level names
     */
    void gather(const PropertyTree& properties, std::vector<std::string>& out) const
    {
        for ( const auto& p : properties )
        {
            auto name = p.second.get_optional<std::string>("name");
            if ( name )
                out.push_back(*name);
            else
                gather(p.second, out);
        }
    }
    /**
     * \brief Searches for a help item with the given name
     */
    const PropertyTree* find ( const PropertyTree& tree,
                               const std::string& name ) const
    {
        auto opt = tree.get_child_optional(name);

        if ( opt && !opt->empty() )
            return &*opt;

        for ( const auto& p : tree )
        {
            auto ptr = find(p.second, name);
            if ( ptr && !ptr->empty() )
                return ptr;
        }

        return nullptr;
    }
};

/**
 * \brief Just repeat what it has been told
 */
class Echo : public melanobot::SimpleAction
{
public:
    Echo(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("echo", settings, parent)
    {
        help = "Repeats \"Text...\"";
        synopsis += " Text...";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg, msg.source->decode(msg.message));
        return true;
    }
};

/**
 * \brief Shows the server the bot is connected to
 */
class ServerHost : public melanobot::SimpleAction
{
public:
    ServerHost(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("server", settings, parent)
    {}

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg, msg.source->description());
        return true;
    }
};

/**
 * \brief Shows one of the given items, at random
 */
class Cointoss : public melanobot::SimpleAction
{
public:
    Cointoss(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("cointoss", settings, parent)
    {
        auto items = settings.get_optional<std::string>("items");
        if ( items )
            default_items = melanolib::string::regex_split(*items, ",\\s*");
        customizable = settings.get("customizable", customizable);

        help = "Get a random element out of ";
        if ( customizable )
        {
            synopsis += " [item...]";
            help += "the given items";
        }
        else
        {
            help += *items;
        }
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::vector<std::string> item_vector;
        if ( customizable )
        {
            item_vector = melanolib::string::comma_split(msg.message);
            if ( item_vector.size() < 2 )
                item_vector = default_items;
        }
        else
            item_vector = default_items;

        if ( !item_vector.empty() )
            reply_to(msg, item_vector[melanolib::math::random(item_vector.size()-1)]);

        return true;
    }

private:
    std::vector<std::string> default_items = { "Heads", "Tails" };
    bool                     customizable = true;
};

/**
 * \brief Fixed reply
 */
class Reply : public melanobot::Handler
{
public:
    Reply(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {
        trigger         = settings.get("trigger", "");
        reply           = read_string(settings, "reply", "");
        regex           = settings.get("regex", 0);
        case_sensitive  = settings.get("case_sensitive", 1);
        direct          = settings.get("direct", 1);
        help            = settings.get("help", help);

        if ( trigger.empty() || reply.empty() )
            throw melanobot::ConfigurationError();

        if ( regex )
        {
            auto flags = std::regex::ECMAScript|std::regex::optimize;
            if ( !case_sensitive )
                flags |= std::regex::icase;
            trigger_regex = std::regex(trigger, flags);
        }
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::CHAT && (!direct || msg.direct);
    }

    /**
     * Extra properties:
     *  * trigger
     *  * name (same as trigger)
     *  * direct
     *  * help
     *  * synopsis
     */
    std::string get_property(const std::string& name) const override
    {
        if ( !help.empty() && (name == "trigger" || name == "name") )
            return trigger;
        else if ( name == "direct" )
            return direct ? "1" : "0";
        else if ( name == "help" )
            return help;
        return Handler::get_property(name);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        if ( regex )
        {
            std::smatch match;
            if ( std::regex_match(msg.message, match, trigger_regex) )
            {
                string::FormattedProperties map = {
                    {"sender", msg.source->decode(msg.from.name)},
                    {"channel", melanolib::string::implode(", ", msg.channels)},
                };
                for ( unsigned i = 0; i < match.size(); i++ )
                    map[std::to_string(i)] = match[i].str();
                on_handle(msg, reply.replaced(map));
                return true;
            }
        }
        else if ( msg.message == trigger )
        {
            on_handle(msg, reply.copy());
            return true;
        }
        else if ( !case_sensitive &&
            melanolib::string::strtolower(msg.message) == melanolib::string::strtolower(trigger) )
        {
            on_handle(msg, reply.copy());
            return true;
        }

        return false;
    }

    virtual void on_handle(const network::Message& msg, string::FormattedString&& reply) const
    {
        reply_to(msg, reply);
    }

private:
    std::string trigger;                ///< Trigger pattern
    string::FormattedString reply;      ///< Reply string
    bool        regex{false};           ///< Whether the trigger is a regex
    bool        case_sensitive{true};   ///< Whether matches are case sensitive
    bool        direct{true};           ///< Whether the input message must be direct
    std::regex  trigger_regex;          ///< Regex for the trigger
    std::string help;                   ///< Help message
};

/**
 * \brief Fixed command
 */
class Command : public Reply
{
public:
    using Reply::Reply;

protected:
    virtual void on_handle(const network::Message& msg, string::FormattedString&& reply) const
    {
        msg.destination->command(network::Command(
            reply.encode(*msg.destination->formatter()), {}, priority));
    }
};

/**
 * \brief Performs an action
 */
class Action : public melanobot::SimpleAction
{
public:
    Action(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("please", settings, parent)
    {
        help = "Make the bot perform a chat action (Roleplay)";
        synopsis += " Action...";
        unauthorized = read_string(settings, "unauthorized", "Won't do!");
        empty = read_string(settings, "empty", "Please what?");
        auth = settings.get("auth", auth);
    }

    std::string get_property(const std::string& name) const override
    {
        if ( name == "auth" )
            return auth;
        return SimpleAction::get_property(name);
    }

protected:

    bool on_handle(network::Message& msg) override
    {
        if ( !auth.empty() && !msg.source->user_auth(msg.from.local_id, auth) )
        {
            reply_to(msg, unauthorized);
            return true;
        }

        using melanolib::string::english;
        // should match \S+|(?:don't be) but it's ambiguous
        static std::regex regex_imperative (
            R"(\s*(\S+(?: be\b)?)\s*(.*))",
            std::regex::ECMAScript|std::regex::optimize|std::regex::icase
        );

        std::smatch match;
        if ( std::regex_match(msg.message, match, regex_imperative) )
        {
            reply_to(msg, network::OutputMessage(
                english.imperate(match[1])+" "+
                english.pronoun_to3rd(match[2], msg.from.name, msg.source->name()),
                true
            ));
        }
        else
        {
            reply_to(msg, empty);
        }
        return true;
    }

private:
    string::FormattedString unauthorized;
    string::FormattedString empty;
    std::string auth;
};

/**
 * \brief Handler showing the time
 */
class Time : public melanobot::SimpleAction
{
public:
    Time(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("time", settings, parent)
    {
        format = settings.get("format", format);
        synopsis += " [time]";
        help = "Shows the time";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg, melanolib::time::strftime(melanolib::time::parse_time(msg.message), format));
        return true;
    }

private:
    std::string format = "%r (UTC)"; ///< Output time format
};

/**
 * \brief Aborts the bot after a connection error
 */
class ErrorAbort : public melanobot::Handler
{
public:
    ErrorAbort(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
        {}

public:
    bool can_handle(const network::Message& msg) const override
    {
        return msg.type == network::Message::ERROR;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        settings::global_settings.put("exit_code", 1);
        melanobot::Melanobot::instance().stop("ErrorAbort", "abort from error message");
        return true;
    }
};

/**
 * \brief Forwards a message to a group
 */
class GroupNotify : public melanobot::SimpleAction
{
public:
    GroupNotify(const Settings& settings, MessageConsumer* parent)
        : SimpleAction(settings.get("notify", "admin"), settings, parent)
    {
        notify = settings.get("notify", notify);

        help = "Message the " + notify + " group";
        synopsis += " Text...";

        admin_message = read_string(settings, "admin_message", "$(-b)$connection$(-): $message");
        echo_message = read_string(settings, "echo_message", "$(1)!" + trigger + "$(-) $message");

        int timeout_seconds = settings.get("timeout", 0);
        if ( timeout_seconds > 0 )
            timeout = std::chrono::duration_cast<network::Duration>(
                std::chrono::seconds(timeout_seconds) );
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        auto props = msg.source->pretty_properties(msg.from);
        props["connection"] = msg.source->config_name();
        props["message"] = msg.source->decode(msg.message);

        network::OutputMessage out_echo(
            echo_message.replaced(props),
            false,
            {},
            priority,
            msg.source->decode(msg.from.name),
            {},
            timeout == network::Duration::zero() ?
                network::Time::max() :
                network::Clock::now() + timeout
        );
        reply_to(msg, out_echo);

        auto rendered_admin_message = admin_message.replaced(props);
        for ( const auto& admin : msg.destination->real_users_in_group(notify) )
        {
            network::OutputMessage out(
                rendered_admin_message,
                false,
                admin.local_id,
                priority,
                msg.source->decode(msg.from.name),
                {},
                timeout == network::Duration::zero() ?
                    network::Time::max() :
                    network::Clock::now() + timeout
            );
            deliver(msg.destination, out);
        }
        return true;
    }

private:
    string::FormattedString admin_message; ///< Message sent to the admins
    string::FormattedString echo_message; ///< Message sent to the channel
    std::string notify = "admin"; ///< Group to be notified
    network::Duration timeout{network::Duration::zero()};///< Output message timeout
};

} // namespace core
#endif // HANDLER_MISC

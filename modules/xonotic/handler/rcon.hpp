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
#ifndef XONOTIC_HANDLER_RCON_HPP
#define XONOTIC_HANDLER_RCON_HPP

#include "melanobot/handler.hpp"
#include "xonotic/xonotic.hpp"

namespace xonotic {

/**
 * \brief Sends a command overriding sv_adminnick
 */
inline void rcon_adminnick(network::Connection* destination,
                           const std::vector<std::string>& command,
                           const std::string& nick)
{
    destination->command({"rcon", {"Melanobot_nick_push"}});
    destination->command({"rcon", {"set", "sv_adminnick", nick+"^3"}});
    destination->command({"rcon", command});
    destination->command({"rcon", {"defer 1 Melanobot_nick_pop"}});
}

/**
 * \brief Send a fixed rcon command to a Xonotic connection
 */
class RconCommand : public melanobot::SimpleAction
{
public:
    RconCommand(const Settings& settings, MessageConsumer* parent)
        : SimpleAction(settings.get("command", settings.data()), settings, parent)
    {
        /// \note it allows the command to be specified in the top-level data
        command = settings.get("command", settings.data());
        if ( command.empty() )
            throw melanobot::ConfigurationError{};
        arguments = settings.get("arguments", arguments);
        if ( arguments )
            synopsis += " argument...";
        /// \todo would be cool to gather help from the server
        ///       or at least from settings
        help = "Performs the Rcon command \"$(dark_cyan)"+command+"$(dark_blue)\"";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        std::vector<std::string> args = { command };
        if ( arguments && !msg.message.empty() )
            args.push_back(msg.message);
        msg.destination->command({"rcon", std::move(args), priority});
        return true;
    }

private:
    std::string command;        ///< Rcon command to be sent
    bool        arguments{true};///< Whether to allow command arguments
};


class XonoticVCallStopBase : public melanobot::SimpleAction
{
public:
    XonoticVCallStopBase(const char* vote, const Settings& settings, MessageConsumer* parent)
        : SimpleAction(vote, settings, parent), vote(vote)
    {
        synopsis += " vote";
        nick = read_string(settings, "nick", "$name");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        string::FormattedProperties props {
            {"name", msg.source->decode(msg.from.name)},
            {"local_id", msg.from.local_id},
            {"channel", melanolib::string::implode(", ", msg.channels)}
        };
        rcon_adminnick(msg.destination, {vote, msg.message},
            nick.replaced(props).encode(*msg.destination->formatter()));
        return true;
    }

private:
    string::FormattedString nick; ///< Nick to use
    const char* vote;
};

/**
 * \brief Calls a vote on the server, changing the admin name
 *        to that of the user calling the vote
 */
class XonoticVCall : public XonoticVCallStopBase
{
public:
    XonoticVCall(const Settings& settings, MessageConsumer* parent)
        : XonoticVCallStopBase("vcall", settings, parent)
    {
        help = "Call a vote on the Xonotic server";
    }
};

/**
 * \brief Stop a vote on the server, changing the admin name
 *        to that of the user calling the vote
 */
class XonoticVStop : public XonoticVCallStopBase
{
public:
    XonoticVStop(const Settings& settings, MessageConsumer* parent)
        : XonoticVCallStopBase("vstop", settings, parent)
    {
        help = "Stop a vote on the Xonotic server";
    }
};

} // namespace xonotic
#endif // XONOTIC_HANDLER_RCON_HPP

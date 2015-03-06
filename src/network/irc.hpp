/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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

#ifndef IRC_HPP
#define IRC_HPP

#include <algorithm>
#include <list>
#include <queue>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "connection.hpp"
#include "irc_buffer.hpp"

#include "melanobot.hpp"

namespace network {
/**
 * \brief Namespace contaning classes and functions handling the IRC protocol
 */
namespace irc {

/**
 * \brief Whether a character is a valid nickname character
 * \see http://tools.ietf.org/html/rfc2812#section-2.3.1
 * \todo currently unused
 */
inline bool is_nickchar(char c)
{
    return std::isalnum(c) || c == '-' ||
        ( c >= 0x5B && c <= 0x60 ) || ( c >= 0x7B && c <= 0x7D );
}

/**
 * \brief Converts a string to lower case
 * \see http://tools.ietf.org/html/rfc2812#section-2.2
 * \todo locale stuff if it's worth the effort
 */
inline std::string strtolower ( std::string string )
{
    std::transform(string.begin(),string.end(),string.begin(),[](char c)->char {
        switch (c) {
            case '[': return '{';
            case ']': return '}';
            case '\\': return '|';
            case '~': return '^';
            default:  return std::tolower(c);
        }
    });
    return string;
}
/**
 * \brief Converts a string to upper case
 * \see http://tools.ietf.org/html/rfc2812#section-2.2
 */
inline std::string strtoupper ( std::string string )
{
    std::transform(string.begin(),string.end(),string.begin(),[](char c)->char {
        switch (c) {
            case '{': return '[';
            case '}': return ']';
            case '|': return '\\';
            case '^': return '~';
            default:  return std::toupper(c);
        }
    });
    return string;
}

/**
 * \brief IRC connection
 */
class IrcConnection : public Connection
{
public:
    /**
     * \brief Create from settings
     */
    static IrcConnection* create(Melanobot* bot, const Settings& settings);

    /**
     * \thread main \lock none
     */
    IrcConnection(Melanobot* bot, const Server& server, const Settings& settings = {});
    
    /**
     * \thread main \lock none
     */
    ~IrcConnection() override;

    /**
     * \thread external \lock none
     */
    void start() override;

    /**
     * \thread external \lock buffer(indirect)
     */
    void stop() override;

    /**
     * \thread ? \lock data
     */
    Server server() const override;

    /**
     * \thread external \lock data(sometimes) buffer(indirect)
     */
    void command ( const Command& cmd ) override;

    /**
     * \thread external \lock data(sometimes) buffer(indirect)
     */
    void say ( const std::string& channel,
        const string::FormattedString& message,
        int priority = 0,
        const Time& timeout = Clock::time_point::max() ) override;

    /**
     * \thread external \lock data(sometimes) buffer(indirect)
     */
    void say_as ( const std::string& channel,
        const string::FormattedString& name,
        const string::FormattedString& message,
        const string::FormattedString& prefix = {},
        int priority = 0,
        const Time& timeout = Clock::time_point::max()  )  override;

    /**
     * \thread ? \lock none
     */
    Status status() const override;

    /**
     * \thread external \lock none
     */
    std::string protocol() const override;

    /**
     * \thread external \lock buffer(indirect) data(indirect)
     */
    void connect() override;

    /**
     * \thread external \lock buffer(indirect)
     */
    void disconnect(const std::string& message = {}) override;

    /**
     * \thread external \lock none
     */
    string::Formatter* formatter() const override;

    /**
     * \thread external \lock none
     * \param mask A list of channels names separated by commas or spaces,
     *             the wildcard \c * is supported
     *             and \c ! matches private messages
     */
    bool channel_mask(const std::vector<std::string>& channels,
                      const std::string& mask) const override;

    /**
     * \brief disconnect and connect
     * \thread external \lock buffer(indirect) data(indirect)
     */
    void reconnect();

    /**
     * \brief Bot's current nick
     * \thread ? \lock data
     */
    std::string nick() const;

    /**
     * \brief Parse :Nick!User@host
     * \see http://tools.ietf.org/html/rfc2812#section-2.3.1
     */
    static user::User parse_prefix(const std::string& prefix);

    /**
     * \brief Get whether a user has the given authorization level
     */
    bool user_auth(const std::string& local_id,
                   const std::string& auth_group) const override;
    /**
     * \brief Update the properties of a user by local_id
     */
    void update_user(const std::string& local_id,
                     const Properties& properties) override;
private:

    friend class Buffer;

    /**
     * \brief Handle a IRC message
     * \thread irc_in \lock data(sometimes) buffer(indirect, sometimes)
     */
    void handle_message(Message line);

    /**
     * \brief Extablish connection to the IRC server
     * \thread irc_in \lock buffer(indirect)
     */
    void login();

    /**
     * \brief AUTH to the server
     * \thread irc_in \lock buffer(indirect)
     */
    void auth();

    /**
     * \brief Read members from the given settings
     */
    void read_settings(const Settings& settings);

    mutable std::mutex mutex;

    Melanobot* bot;

    /**
     * \brief IRC server to connect to
     */
    Server main_server;

    /**
     * \brief Server the bot is connected to
     * \todo
     */
    Server current_server;

    /**
     * \brief Command buffer
     */
    Buffer buffer;

    /**
     * \brief Network/Bouncer password
     */
    std::string server_password;
    /**
     * \brief Contains the IRC features advertized by the server
     *
     * As seen on 005 (RPL_ISUPPORT)
     * \see http://www.irc.org/tech_docs/005.html
     */
    Properties server_features;
    /**
     * \brief Current bot nick
     */
    std::string current_nick;
    /**
     * \brief Current bot nick (normalized to lowercase)
     */
    std::string current_nick_lowecase;
    /**
     * \brief Nick that should be used
     */
    std::string preferred_nick;
    /**
     * \brief Nick used by the latest NICK command
     */
    std::string attempted_nick;
    /**
     * \brief Nick used to AUTH
     */
    std::string auth_nick;
    /**
     * \brief Password used to AUTH
     */
    std::string auth_password;
    /**
     * \brief Modes to set after AUTH
     */
    std::string modes;
    /**
     * \brief Input formatter
     */
    string::Formatter* formatter_ = nullptr;
    /**
     * \brief List of channels to join
     */
    std::list<std::string> channels_to_join;
    /**
     * \brief Connection status
     */
    AtomicStatus connection_status;


    user::UserManager user_manager;
    user::AuthSystem  auth_system;
};

} // namespace network::irc
} // namespace network
#endif // IRC_HPP
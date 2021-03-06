/**
 * \file
 * \brief This file contains definitions for handlers which are pretty useless
 *
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
#ifndef FUN_HANDLERS_HPP
#define FUN_HANDLERS_HPP
#include <unordered_map>

#include <boost/filesystem.hpp>

#include "core/handler/bridge.hpp"
#include "web/handler/web-api.hpp"
#include "melanolib/math/random.hpp"
#include "melanolib/time/time_string.hpp"

namespace fun {

/**
 * \brief Handler translating between morse and latin
 */
class Morse : public melanobot::SimpleAction
{
public:
    Morse(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("morse", settings, parent)
    {
        synopsis += " text|morse";
        help = "Converts between ASCII and Morse code";
    }

protected:
    bool on_handle(network::Message& msg) override;

private:
    static std::unordered_map<char, std::string> morse;
};

/**
 * \brief Turns ASCII characters upside-down
 */
class ReverseText : public melanobot::SimpleAction
{
public:
    ReverseText(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("reverse", settings, parent)
    {
        synopsis += " text";
        help = "Turns ASCII upside-down";
    }

protected:
    bool on_handle(network::Message& msg) override;

private:
    static std::unordered_map<char, std::string> reverse_ascii;
};

/**
 * \brief Searches for a Chuck Norris joke
 */
class ChuckNorris : public web::SimpleJson
{
public:
    ChuckNorris(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("norris", settings, parent)
    {
        synopsis += " [name]";
        help = "Shows a Chuck Norris joke from http://icndb.com";
    }

protected:
    bool on_handle(network::Message& msg) override;

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        /// \todo convert html entities
        reply_to(msg, parsed.get("value.joke", ""));
    }

private:
    std::string api_url = "http://api.icndb.com/jokes/random";
};

/**
 * \brief Draws a pretty My Little Pony ASCII art
 * \note Very useful to see how the bot handles flooding
 * \note Even more useful to see pretty ponies ;-)
 * \see https://github.com/mbasaglia/ASCII-Pony
 */
class RenderPony : public melanobot::SimpleAction
{
public:
    RenderPony(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("render_pony", settings, parent)
    {
        synopsis += " pony";
        help = "Draws a pretty pony /)^3^(\\";
        pony_path = settings.get("path", pony_path);
    }

protected:
    bool on_handle(network::Message& msg) override;

private:
    std::string pony_path;
};

/**
 * \brief Shows a countdown to the next My Little Pony episode
 */
class PonyCountDown : public web::SimpleJson
{
public:
    PonyCountDown(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("nextpony", settings, parent)
    {
        api_url = settings.get("url", api_url);
        reply = read_string(settings, "found", "$time_delta until $(-b)$title$(-) (S${season}E${episode})");
        not_found = read_string(settings, "not_found", "Next episode: not soon enough D:");
        help = "Pony countdown ("+api_url+")";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg, web::Request("GET", api_url));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        if ( parsed.empty() )
            return json_failure(msg);

        Properties map;
        map["title"] = parsed.get("name", "");
        map["season"] = melanolib::string::to_string(parsed.get("season", 0), 2);
        map["episode"] = melanolib::string::to_string(parsed.get("episode", 0), 2);
        map["duration"] = parsed.get("duration", "");
        melanolib::time::DateTime time = melanolib::time::parse_time(parsed.get("time", ""));
        melanolib::time::DateTime now;
        auto delta = time > now ? time - now : now - time;
        map["time_delta"] = melanolib::time::duration_string(delta);

        reply_to(msg, reply.replaced(map));
    }

    void json_failure(const network::Message& msg) override
    {
        reply_to(msg, not_found.copy());
    }

private:
    std::string api_url = "http://api.ponycountdown.com/next";
    string::FormattedString reply;
    string::FormattedString not_found;
};

/**
 * \brief Shows a pony face
 */
class PonyFace : public web::SimpleJson
{
public:
    PonyFace(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("ponyface", settings, parent)
    {
        api_url = settings.get("url", api_url);
        not_found = read_string(settings, "not_found", "Pony not found http://ponyfac.es/138/full");
        help = "Pony face ("+api_url+")";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg, web::Request("GET",
            api_url+"tag:"+web::urlencode(msg.message)
        ));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        if ( parsed.empty() )
            return json_failure(msg);
        auto faces = parsed.get_child("faces", {});
        if ( !faces.empty() )
        {
            auto face = faces.get_optional<std::string>(
                std::to_string(melanolib::math::random(faces.size()-1))+".image");
            if ( face )
                reply_to(msg, *face);
            return;
        }
        return json_failure(msg);
    }

    void json_failure(const network::Message& msg) override
    {
        reply_to(msg, not_found);
    }

private:
    std::string api_url = "http://ponyfac.es/api.json/";
    string::FormattedString not_found;
};

/**
 * \brief Answers direct questions
 */
class AnswerQuestions : public melanobot::Handler
{
public:
    AnswerQuestions(const Settings& settings, MessageConsumer* parent)
        : Handler(settings, parent)
    {
        direct    = settings.get("direct", direct);
    }

    bool can_handle(const network::Message& msg) const override
    {
        return msg.direct && !msg.message.empty() && msg.message.back() == '?';
    }

protected:
    bool on_handle(network::Message& msg) override;

private:
    bool direct = true;

    /// Answers corresponding to yes or no
    static std::vector<std::string> category_yesno;
    /// Generic and unsatisfying answers
    static std::vector<std::string> category_dunno;

    /// Answers to some time in the past
    static std::vector<std::string> category_when_did;
    /// Generic answers to when
    static std::vector<std::string> category_when;
    /// Answers to some time in the future
    static std::vector<std::string> category_when_will;

    /**
     * \brief Selects a random answer from a set of categories
     */
    void random_answer(network::Message& msg,
                       const std::vector<std::vector<std::string>*>& categories) const;
};

/**
 * \brief Slaps someone
 */
class Slap : public melanobot::SimpleAction
{
public:
    Slap(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("slap", settings, parent)
    {
        synopsis += " victim";
        help = "Slap the victim";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        reply_to(msg, network::OutputMessage(
            string::FormattedString() << "slaps " << msg.message,
            true
        ));
        return true;
    }
};

/**
 * \brief Like BridgeChat but more colorful
 */
class RainbowBridgeChat : public core::BridgeChat
{
public:
    using core::BridgeChat::BridgeChat;

protected:
    bool on_handle(network::Message& msg) override;
};

/**
 * \brief Discordian Calendar
 */
class Discord : public melanobot::SimpleAction
{
public:
    Discord(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("discord", settings, parent)
    {
        synopsis += " [time]";
        help = "Show the Discordian date";
        format = read_string(settings, "format",
            "$day_name, the $season_day day of $season in the YOLD $yold");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        static std::vector<std::string> day_names {
            "Sweetmorn", "Boomtime", "Pungenday", "Prickle-Prickle", "Setting Orange"};
        static std::vector<std::string> season_names {
            "Chaos", "Discord", "Confusion", "Bureaucracy", "The Aftermath"};

        melanolib::time::DateTime dt = melanolib::time::parse_time(msg.message);
        int day = dt.year_day();
        int year = dt.year() + 1166;
        if ( year % 4 == 2 && day >= 59 )
            day--;

        Properties discord {
            {"day_name", day_names[day % day_names.size()]},
            {"season_day", std::to_string(day % 73 + 1)+melanolib::string::english.ordinal_suffix(day % 73 + 1)},
            {"season", season_names[(day/73) % season_names.size()]},
            {"yold", std::to_string(year)}
        };

        reply_to(msg, format.replaced(discord));
        return true;
    }

private:
    string::FormattedString format;
};

/**
 * \brief Very polite remarks
 */
class Insult : public melanobot::SimpleAction
{
public:
    Insult(const Settings& settings, MessageConsumer* parent);

protected:
    bool on_handle(network::Message& msg) override;

private:
    std::string random_adjectives() const;

    int min_adjectives = 1;
    int max_adjectives = 3;
};


/**
 * \brief Handler translating between morse and latin
 */
class Stardate : public melanobot::SimpleAction
{
public:
    using unix_t = int64_t;
    using stardate_t = long double;

    Stardate(const Settings& settings, MessageConsumer* parent)
        : SimpleAction("stardate", settings, parent)
    {
        synopsis += " [time|stardate]";
        help = "Converts between stardates and Gregorian dates";
    }

    static constexpr unix_t stardate_to_unix(stardate_t stardate)
    {
        return (stardate - unix_epoch_stardate) * seconds_per_stardate;
    }

    static constexpr stardate_t unix_to_stardate(unix_t unix)
    {
        return unix / seconds_per_stardate + unix_epoch_stardate;
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        static std::regex stardate(R"(^\s*(\d+\.\d+)\s*$)",
            std::regex::ECMAScript|std::regex::optimize
        );
        using namespace melanolib::time;


        std::smatch match;
        if ( std::regex_match(msg.message, match, stardate) )
        {
            auto date =
                DateTime(1970, Month::JANUARY, days(1), hours(0), minutes(0)) +
                seconds(stardate_to_unix(std::stold(match[1])))
            ;
            reply_to(msg, format_char(date, 'r'));
        }
        else
        {
            auto stardate = unix_to_stardate(parse_time(msg.message).unix());
            reply_to(msg, std::to_string(stardate));
        }

        return true;
    }

    static constexpr stardate_t seconds_per_stardate = 31536;
    static constexpr stardate_t unix_epoch_stardate = -353260.7;
};

/**
 * \brief Shows an image from random.cat
 */
class RandomCat : public web::SimpleJson
{
public:
    RandomCat(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("meow", settings, parent)
    {
        help = "Shows an image from random.cat";
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        request_json(msg, web::Request("GET", web::Uri(api_url)));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        reply_to(msg, parsed.get("file", ""));
    }

private:
    std::string api_url = "http://random.cat/meow";
};

class E926 : public web::SimpleJson
{
public:
    E926(const Settings& settings, MessageConsumer* parent)
        : SimpleJson("furry", settings, parent)
    {
        image_type = settings.get("image_type", image_type);
        endpoint = settings.get("endpoint", endpoint);

        help = "Shows a " + image_type + " from " + endpoint;

        fixed_tags = settings.get("fixed_tags", fixed_tags);
        if ( !fixed_tags.empty() )
            fixed_tags += ' ';

        reply = read_string(settings, "reply",
            "Image by$(-b)$(for person $artist) $person$(endfor)$(-): $file_url");
        not_found = read_string(settings, "not_found",
            "Didn't find any $image_type for $query");
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        web::Uri uri(endpoint + "/post/index.json", {
            {"tags",  fixed_tags+msg.message},
        });
        if ( limit )
            uri.query["limit"] = std::to_string(limit);

        request_json(msg, web::Request("GET", uri));
        return true;
    }

    void json_success(const network::Message& msg, const Settings& parsed) override
    {
        if ( !parsed.size() )
            return json_failure(msg);

        auto it = parsed.begin();
        std::advance(it, melanolib::math::random(parsed.size() - 1));

        Properties map;
        map["url_base"] = endpoint;
        map["url_base"] = endpoint;
        map["image_type"] = image_type;
        map["query"] = msg.message;

        reply_to(msg, reply.replaced(map).replaced(it->second));
    }

    void json_failure(const network::Message& msg) override
    {
        Properties map;
        map["image_type"] = image_type;
        map["query"] = msg.message;

        reply_to(msg, not_found.replaced(map));
    }

private:
    std::string endpoint = "https://e926.net";
    std::string image_type = "furry pic";
    std::string fixed_tags = "order:score -flash -type:webm";
    int limit = 0;
    string::FormattedString reply;
    string::FormattedString not_found;
    std::map<std::string, std::string> ratings = {
        {"e", "explicit"},
        {"q", "questionable"},
        {"s", "safe"},
    };
};

} // namespace fun
#endif // FUN_HANDLERS_HPP

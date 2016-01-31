/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2016 Mattia Basaglia
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
#ifndef MELANOBOT_MODULE_GITHUB_CONNECTION_HPP
#define MELANOBOT_MODULE_GITHUB_CONNECTION_HPP

#include "github-listeners.hpp"
#include "network/async_service.hpp"
#include "repository.hpp"

namespace github {


/**
 * \todo Auth and stuff
 */
class GitHubEventSource : public AsyncService
{
public:
    void initialize(const Settings& settings) override;

    ~GitHubEventSource();

    void stop() override;

    void start() override;


private:
    void poll();

    std::vector<Repository> repositories;
    std::string api_url = "https://api.github.com";
    melanolib::time::Timer timer; ///< Polling timer
    std::vector<std::unique_ptr<GitHubEventListener>> listeners;
};

} // namespace github
#endif // MELANOBOT_MODULE_GITHUB_CONNECTION_HPP

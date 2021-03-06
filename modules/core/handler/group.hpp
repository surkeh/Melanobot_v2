/**
 * \file
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
#ifndef HANDLER_SIMPLE_GROUP_HPP
#define HANDLER_SIMPLE_GROUP_HPP

#include "melanobot/handler.hpp"

namespace core {

/**
 * \brief Base class for group-like handlers
 */
class AbstractGroup : public melanobot::Handler
{
public:
    using melanobot::Handler::Handler;

    void initialize() override
    {
        for ( const auto& h : children )
            h->initialize();
    }

    void finalize() override
    {
        for ( const auto& h : children )
            h->finalize();
    }

    void populate_properties(const std::vector<std::string>& properties,
                             PropertyTree& output) const override;


    void add_handler(std::unique_ptr<melanobot::Handler>&& handler) override
    {
        children.push_back(std::move(handler));
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        for ( const auto& h : children )
            if ( h->handle(msg) )
                return true;
        return false;
    }

    /**
     * \brief Creates children from settings
     * \param child_settings    Settings for all the children
     * \param default_settings  Settings to fallback to for every child
     */
    void add_children(Settings child_settings,
                      const Settings& default_settings={});

    /**
     * \brief Called whenever a child is added to the group
     */
    virtual void on_add_child(Handler& child, const Settings& settings){}

    std::vector<std::unique_ptr<Handler>> children;  ///< Contained handlers

    friend class AbstractActionGroup;
};

/**
 * \brief Base class for handlers which act as a group and as a SimpleAction
 */
class AbstractActionGroup : public melanobot::SimpleAction
{
public:
    AbstractActionGroup(const std::string& default_trigger,
                        const Settings& settings,
                        MessageConsumer* parent)
    : SimpleAction(default_trigger, settings, parent),
      group({}, this) {}

    void initialize() override
    {
        group.initialize();
    }

    void finalize() override
    {
        group.finalize();
    }

    void populate_properties(const std::vector<std::string>& properties,
                             PropertyTree& output) const override
    {
        group.populate_properties(properties, output);
        SimpleAction::populate_properties(properties, output);
    }

protected:
    bool on_handle(network::Message& msg) override
    {
        return group.on_handle(msg);
    }

    void add_children(Settings child_settings,
                      const Settings& default_settings={})
    {
        group.add_children(child_settings, default_settings);
    }

    const std::vector<std::unique_ptr<Handler>>& children() const
    {
        return group.children;
    }

    void add_handler(std::unique_ptr<melanobot::Handler>&& handler) override
    {
        group.children.push_back(std::move(handler));
    }

private:
    AbstractGroup group;
};

/**
 * \brief A simple group of actions which share settings
 */
class Group : public AbstractActionGroup
{
public:
    Group(const Settings& settings, MessageConsumer* parent);

    bool can_handle(const network::Message& msg) const override;

    std::string get_property(const std::string& name) const override;

    /**
     * \brief Checks if a message is authorized to be executed by this message
     */
    bool authorized(const network::Message& msg) const
    {
        return auth.empty() || msg.source->user_auth(msg.from.local_id, auth);
    }

protected:
    bool on_handle(network::Message& msg) override;

    void output_filter(network::OutputMessage& output) const override
    {
        output.prefix = string::FormattedString(prefix) << output.prefix;
    }

    /**
     * \brief Authorization group required for a user message to be handled
     */
    std::string          auth;
    std::string          channels;          ///< Channel filter
    network::Connection* source = nullptr;  ///< Accepted connection (Null => all connections)
    std::string          name;              ///< Name to show in help
    std::string          help_group;        ///< Selects whether to be shown in help
    bool                 pass_through=false;///< Whether it should keep processing the message after a match
    std::string          prefix;            ///< Output message prefix \todo FormattedString
};

/**
 * \brief Handles a list of elements (base class)
 * \note Derived classes shall provide the property \c list_name
 *       which contains a human-readable name of the list,
 *       used for descriptions of the handler.
 */
class AbstractList : public AbstractActionGroup
{
public:
    /**
     * \param default_trigger   Default trigger/group name
     * \param clear             Whether to allow clearing the list
     * \param settings          Handler settings
     */
    AbstractList(const std::string& default_trigger, bool clear,
                 const Settings& settings, MessageConsumer* parent);

    /**
     * \brief Adds \c element to the list
     * \return \b true on success
     */
    virtual bool add(const std::string& element) = 0;
    /**
     * \brief Removes \c element from the list
     * \return \b true on success
     */
    virtual bool remove(const std::string& element) = 0;
    /**
     * \brief Removes all elements from the list
     * \return \b true on success
     */
    virtual bool clear() = 0;

    /**
     * \brief Returns a vector containing all the elements of the string
     */
    virtual std::vector<std::string> elements() const = 0;

    std::string get_property(const std::string& name) const override
    {
        if ( name == "help" )
            return "Manages "+get_property("list_name");
        return SimpleAction::get_property(name);
    }

protected:
    bool on_handle(network::Message& msg) override;

    std::string edit; ///< User group allowed to edit the list
};

/**
 * \brief Very basic group with preset handlers, configurable from the config
 */
class PresetGroup : public AbstractGroup
{
public:
    PresetGroup( const std::initializer_list<std::string>& preset,
                 const Settings& settings, MessageConsumer* parent)
        : AbstractGroup(settings, parent)
    {
        add_children(settings::merge_copy(settings,
            settings::from_initializer(preset), false));
    }

    bool can_handle(const network::Message&) const override { return true; }
};

/**
 * \brief A group muticasting to its children
 * (which should be SimpleActions with a non-empty trigger)
 */
class Multi : public AbstractActionGroup
{
public:
    Multi(const Settings& settings, MessageConsumer* parent);

    bool can_handle(const network::Message& msg) const override;

    bool handle(network::Message& msg) override
    {
        return Handler::handle(msg);
    }

protected:
    bool on_handle(network::Message& msg) override;

private:
    std::vector<std::string> prefixes;
};

/**
 * \brief Group that gets enabled only if a global config value has a certain value
 */
class IfSet : public AbstractGroup
{
public:
    IfSet (const Settings& settings, MessageConsumer* parent);

    bool can_handle(const network::Message&) const override
    {
        return !children.empty();
    }
};

/**
 * \brief Group that forwards messages randomly to one of its children.
 * Children have a weight of 1 by default but they can define random_weight
 * to change this
 */
class RandomDispatch : public AbstractGroup
{
public:
    RandomDispatch(const Settings& settings, MessageConsumer* parent);

    bool can_handle(const network::Message& msg) const override;

protected:
    bool on_handle(network::Message& msg) override;
    void on_add_child(Handler& child, const Settings& settings) override;


private:
    RandomDispatch(const Settings& settings, MessageConsumer* parent, bool);
    float total_wight() const;

    std::vector<float> weights;
};

} // namespace core
#endif // HANDLER_SIMPLE_GROUP

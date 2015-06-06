/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
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
#ifndef PYTHON_MODULES_HPP
#define PYTHON_MODULES_HPP

#include "boost-python.hpp"
#include "settings.hpp"
#include "message/input_message.hpp"
#include "script_variables.hpp"
#include "melanobot.hpp"
#include "network/connection.hpp"

namespace python {

/**
 * \brief Return a functor converting a pointer to member using Converter::convert
 */
template<class Class, class Member>
    auto convert_member(Member Class::*member)
    {
        return [member](const Class& obj) {
            boost::python::object out;
            Converter::convert(obj.*member, out);
            return out;
        };
    }

/**
 * \brief Shothand to make functions that return pointer managed by C++
 */
template<class Func>
    auto return_pointer(Func&& func)
    {
        using namespace boost::python;
        return make_function(std::forward<Func>(func),
            return_value_policy<reference_existing_object>());
    }

/**
 * \brief Shothand to make functions that return a reference that should be copied
 */
template<class Func>
    auto return_copy(Func&& func)
    {
        using namespace boost::python;
        return make_function(std::forward<Func>(func),
            return_value_policy<return_by_value>());
    }

/**
 * \brief Creates a shared_ptr to a user which will update it upon destruction
 */
inline std::shared_ptr<user::User> make_shared_user(const user::User& user)
{
    std::string starting_id = user.local_id;
    return std::shared_ptr<user::User>(new user::User(user),
        [starting_id](user::User* ptr) {
            if ( ptr->origin && !starting_id.empty() )
                ptr->origin->update_user(starting_id, *ptr);
            delete ptr;
        });
}

/**
 * \brief Namespace corresponding to the python module \c melanobot
 */
namespace melanobot {

BOOST_PYTHON_MODULE(melanobot)
{
    using namespace boost::python;

    def("data_file", &settings::data_file);
    def("data_file", [](const std::string& path) { return settings::data_file(path); } );

    class_<user::User, std::shared_ptr<user::User>>("User",no_init)
        .def_readwrite("name",&user::User::name)
        .def_readwrite("host",&user::User::host)
        .def_readonly("local_id",&user::User::local_id)
        .def_readwrite("global_id",&user::User::global_id)
        .add_property("channels",convert_member(&user::User::channels))
        .def("__getattr__",&user::User::property)
        .def("__setattr__",[](user::User& user, const std::string& property, object val) {
            user.properties[property] = extract<std::string>(val);
        })
    ;

    /// \todo readonly or readwrite?
    class_<network::Message>("Message",no_init)
        .def_readwrite("raw",&network::Message::raw)
        .def_readonly("params",convert_member(&network::Message::params))

        .def_readwrite("message",&network::Message::message)
        .def_readonly("channels",convert_member(&network::Message::channels))
        .def_readwrite("direct",&network::Message::direct)
        .add_property("user",[](const network::Message& msg){
            return make_shared_user(msg.from);
        })
        .def_readonly("victim",[](const network::Message& msg){
            return make_shared_user(msg.victim);
        })
        .def_readonly("source",&network::Message::source)
        .def_readonly("destination",&network::Message::destination)
    ;

    class_<Melanobot,Melanobot*,boost::noncopyable>("Melanobot",no_init)
        .def("stop",&Melanobot::stop)
        .def("connection",return_pointer(&Melanobot::connection))
    ;

    class_<color::Color12>("Color")
        .def(init<std::string>())
        .def(init<color::Color12::BitMask>())
        .def(init<color::Color12::Component,color::Color12::Component,color::Color12::Component>())
        .add_property("valid",&color::Color12::is_valid)
        .add_property("red",&color::Color12::red)
        .add_property("green",&color::Color12::green)
        .add_property("blue",&color::Color12::blue)
        .def("hsv",&color::Color12::hsv).staticmethod("hsv")
        .def("blend",&color::Color12::blend).staticmethod("blend")
        .def("__str__",[](const color::Color12& col){
            object main = import("__main__");
            dict main_namespace = extract<dict>(main.attr("__dict__"));
            if ( !main_namespace.has_key("formatter") )
                return object(str());
            return main_namespace["formatter"].attr("convert")(col);
        })
        .def("__add__",[](const color::Color12& lhs, const str& rhs) {
            return str(lhs)+rhs;
        })
    ;

    class_<string::Formatter,string::Formatter*,boost::noncopyable>("Formatter",no_init)
        .def("__init__", make_constructor([](const std::string& name) {
            return string::Formatter::formatter(name);
        }))
        .add_property("name",[](string::Formatter* fmt) {
            return fmt ? fmt->name() : "";
        })
        .def("convert",[](string::Formatter* fmt, const color::Color12& col) {
            return fmt ? fmt->color(col) : "";
        })
        .def("convert",[](string::Formatter* fmt, const std::string& str) {
            return str;
        })
    ;

    class_<network::Connection,network::Connection*,boost::noncopyable>("Connection",no_init)
        .add_property("name",return_copy(&network::Connection::config_name))
        .add_property("description",&network::Connection::description)
        .add_property("protocol",&network::Connection::protocol)
        .add_property("formatter",return_pointer(&network::Connection::formatter))
        .def("user",[](network::Connection* conn, const std::string& local_id){
            return make_shared_user(conn->get_user(local_id));
        })
        /// \todo Expose Command
        .def("command",[](network::Connection* conn,const std::string& command){
            conn->command({command});
        })
        /// \todo Expose OutputMessage and say()
        .def("connect",&network::Connection::connect)
        .def("disconnect",&network::Connection::disconnect)
        .def("reconnect",&network::Connection::reconnect)
    ;

}

} // namespace melanobot
} // namespace python
#endif // PYTHON_MODULES_HPP
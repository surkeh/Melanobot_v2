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

#include "module/melanomodule.hpp"
#include "handler/misc.hpp"
#include "handler/inventory.hpp"

/**
 * \brief Initializes the lists module
 */
MELANOMODULE_ENTRY_POINT module::Melanomodule melanomodule_lists_metadata()
{
    return {"lists", "Manage ToDo Lists and similar"};
}

MELANOMODULE_ENTRY_POINT void melanomodule_lists_initialize(const Settings&)
{
    module::register_handler<lists::FixedList>("FixedList");
    module::register_handler<lists::DynamicReply>("DynamicReply");
    module::register_handler<lists::DynamicReplyManager>("DynamicReplyManager");

    module::register_handler<lists::InventoryManager>("InventoryManager");
    module::register_handler<lists::InventoryPut>("InventoryPut");
    module::register_handler<lists::InventoryTake>("InventoryTake");
}

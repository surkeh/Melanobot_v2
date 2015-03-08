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
#include "color.hpp"

#include <algorithm>

namespace color {

Color12 nocolor;
Color12 black           = Color12(0x0, 0x0, 0x0);
Color12 red             = Color12(0xf, 0x0, 0x0);
Color12 green           = Color12(0x0, 0xf, 0x0);
Color12 yellow          = Color12(0xf, 0xf, 0x0);
Color12 blue            = Color12(0x0, 0x0, 0xf);
Color12 magenta         = Color12(0xf, 0x0, 0xf);
Color12 cyan            = Color12(0x0, 0xf, 0xf);
Color12 white           = Color12(0xf, 0xf, 0xf);
Color12 silver          = Color12(0xc, 0xc, 0xc);
Color12 gray            = Color12(0x8, 0x8, 0x8);
Color12 dark_red        = Color12(0x8, 0x0, 0x0);
Color12 dark_green      = Color12(0x0, 0x8, 0x0);
Color12 dark_yellow     = Color12(0x8, 0x8, 0x0);
Color12 dark_blue       = Color12(0x0, 0x0, 0x8);
Color12 dark_magenta    = Color12(0x8, 0x0, 0x8);
Color12 dark_cyan       = Color12(0x0, 0x8, 0x8);

Color12::Component Color12::to_4bit() const
{
    if ( !valid )
        return 0xf0;

    Component color = 0;

    Component cmax = std::max({r,g,b});
    Component cmin = std::min({r,g,b});
    Component delta = cmax-cmin;

    if ( delta > 0 )
    {
        float hue = 0;
        if ( r == cmax )
            hue = float(g-b)/delta;
        else if ( g == cmax )
            hue = float(b-r)/delta + 2;
        else if ( b == cmax )
            hue = float(r-g)/delta + 4;

        float sat = float(delta)/cmax;
        if ( sat >= 0.3 )
        {
            if ( hue < 0 )
                hue += 6;

            if ( hue <= 0.5 )      color = 0b001; // red
            else if ( hue <= 1.5 ) color = 0b011; // yellow
            else if ( hue <= 2.5 ) color = 0b010; // green
            else if ( hue <= 3.5 ) color = 0b110; // cyan
            else if ( hue <= 4.5 ) color = 0b100; // blue
            else if ( hue <= 5.5 ) color = 0b101; // magenta
            else                   color = 0b001; // red
        }
        else if ( cmax > 7 )
            color = 7;

        if ( cmax > 9 )
            color |= 0b1000; // bright
    }
    else
    {
        if ( cmax > 0xc )
            color = 0b1111; // white
        else if ( cmax > 0x8 )
            color = 0b0111; // silver
        else if ( cmax > 0x4 )
            color = 0b1000; // gray
        else
            color = 0b0000; // black
    }

    return color;
}

Color12 Color12::from_4bit(Component color)
{
    switch(color)
    {
        case 0b0000: return black;
        case 0b0001: return dark_red;
        case 0b0010: return dark_green;
        case 0b0011: return dark_yellow;
        case 0b0100: return dark_blue;
        case 0b0101: return dark_magenta;
        case 0b0110: return dark_cyan;
        case 0b0111: return silver;
        case 0b1000: return gray;
        case 0b1001: return color::red;
        case 0b1010: return color::green;
        case 0b1011: return yellow;
        case 0b1100: return color::blue;
        case 0b1101: return magenta;
        case 0b1110: return cyan;
        case 0b1111: return white;
        default:     return nocolor;
    }
}

std::string Color12::to_html() const
{
    return std::string()+'#'+component_to_hex(r)+component_to_hex(g)+component_to_hex(b);
}

} // namespace color

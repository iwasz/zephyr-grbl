/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once

namespace grbl {

enum class JogDirection { yPositive, yNegative, xPositive, xNegative };
void jog (JogDirection dir);

} // namespace grbl
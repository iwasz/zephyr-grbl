/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <gsl/gsl>

namespace sd {

void init ();
void executeLine (gsl::czstring fileName);
int lsdir (gsl::czstring path);

} // namespace sd
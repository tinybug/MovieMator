/*
 * Copyright (c) 2016-2019 EffectMatrix Inc.
 * Author: wyl1987527 <wyl1987527@163.com>
 * Author: Author: fuyunhuaxin <2446010587@qq.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FILTERDOCK_GLOBAL_H
#define FILTERDOCK_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FILTERDOCK_LIBRARY)
#  define FILTERDOCKSHARED_EXPORT Q_DECL_EXPORT
#else
#  define FILTERDOCKSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // FILTERSELECTDOCK_GLOBAL_H

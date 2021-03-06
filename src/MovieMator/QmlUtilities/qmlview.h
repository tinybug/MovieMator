/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#ifndef QMLVIEW_H
#define QMLVIEW_H

#include "qmlutilities_global.h"

#include <QObject>
#include <QPoint>

class QWindow;

class QMLUTILITIESSHARED_EXPORT QmlView : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QPoint pos READ pos)

public:
    explicit QmlView(QWindow* qview);
    QPoint pos();

private:
    QWindow* m_qview;
};

#endif // QMLVIEW_H

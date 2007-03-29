/****************************************************************************
**
** Copyright (C) 2006 Trolltech AS. All rights reserved.
**
** This file is part of the documentation of Qt. It was originally
** published as part of Qt Quarterly.
**
** This file may be used under the terms of the GNU General Public License
** version 2.0 as published by the Free Software Foundation or under the
** terms of the Qt Commercial License Agreement. The respective license
** texts for these are provided with the open source and commercial
** editions of Qt.
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtGui>

#include "faderwidget.h"

FaderWidget::FaderWidget(QWidget *parent)
    : QWidget(parent)
{
    if (parent) {
        startColor = endColor = parent->palette().window().color();
    } else {
        startColor = endColor = Qt::white;
    }

    currentAlpha = 0;
    duration = 333;
    fadeOut = false;

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));

    setAttribute(Qt::WA_DeleteOnClose);
    resize(parent->size());
}

void FaderWidget::start()
{
    currentAlpha = 255;
    fadeOut = false;
    timer->start(33);
    show();
}

void FaderWidget::startFadeOut()
{
    currentAlpha = 0;
    fadeOut = true;
    timer->start(33);
    show();
}

void FaderWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    QColor semiTransparentColor = startColor;
    semiTransparentColor.setAlpha(currentAlpha);
    painter.fillRect(rect(), semiTransparentColor);

    if (fadeOut) {
        currentAlpha += 255 * timer->interval() / duration;
        if (currentAlpha >= 255) {
            timer->stop();
            emit done( (QWidget *)parent() );
            close();
        }
    } else {
        currentAlpha -= 255 * timer->interval() / duration;
        if (currentAlpha <= 0) {
            timer->stop();
            emit done( (QWidget *)parent() );
            close();
        }
    }
}

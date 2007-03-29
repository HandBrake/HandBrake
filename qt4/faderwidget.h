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

#ifndef FADERWIDGET_H
#define FADERWIDGET_H

#include <QWidget>

class QTimer;

class FaderWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor fadeColor READ fadeColor WRITE setFadeColor)
    Q_PROPERTY(int fadeDuration READ fadeDuration WRITE setFadeDuration)

public:
    FaderWidget(QWidget *parent);

    QColor fadeColor() const { return startColor; }
    void setFadeColor(const QColor &newColor) { startColor = newColor; }

    int fadeDuration() const { return duration; }
    void setFadeDuration(int milliseconds) { duration = milliseconds; }

    void start();
    void startFadeOut();

signals:
    void done(QWidget *w);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QTimer *timer;
    QColor startColor, endColor;
    bool fadeOut;
    int currentAlpha;
    int duration;
};

#endif

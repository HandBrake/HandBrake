#ifndef ENCODEWIDGET_H
#define ENCODEWIDGET_H

#include <QtGui>

#include "ui_encodewidget.h"

class EncodeWidget : public QWidget, private Ui::EncodeWidget
{
    Q_OBJECT
public:
    EncodeWidget( QWidget *parent = 0 );

signals:
    void convert();

public slots:
    void setModel( QStandardItemModel *m );
    void changed();
};

#endif // ENCODEWIDGET_H

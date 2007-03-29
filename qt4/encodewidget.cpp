#include <QtGui>

#include "encodewidget.h"

EncodeWidget::EncodeWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    convertButton->setDisabled( true );

    connect(convertButton, SIGNAL(clicked()), this, SIGNAL(convert()));
}

void EncodeWidget::setModel( QStandardItemModel *m )
{
    titleTree->setModel( m );
    titleTree->setSelectionMode( QAbstractItemView::NoSelection );
    titleTree->setRootIsDecorated( false );
    titleTree->setFocusPolicy( Qt::NoFocus );

    connect(titleTree->model(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), SLOT(changed()));
}

void EncodeWidget::changed()
{
    bool enable = false;

    for( int row = 0; row < titleTree->model()->rowCount(); ++row )
    {
        QStandardItem *si = qobject_cast<QStandardItemModel *>(titleTree->model())->item( row, 0 );
        if( si->checkState() == Qt::Checked )
        {
            enable = true;
        }
    }

    convertButton->setEnabled( enable );
}

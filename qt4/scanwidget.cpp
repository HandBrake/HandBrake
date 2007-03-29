#include <QtGui>
#include <QtDBus>

#include "scanwidget.h"

#define HAL_SERVICE "org.freedesktop.Hal"
#define HAL_PATH_MANAGER "/org/freedesktop/Hal/Manager"
#define HAL_PATH_DEVICE "/org/freedesktop/Hal/Device"
#define HAL_OBJECT_MANAGER "org.freedesktop.Hal.Manager"
#define HAL_OBJECT_DEVICE "org.freedesktop.Hal.Device"

ClickedField::ClickedField(QRadioButton *b, QObject *parent)
    : QObject(parent)
{
    rb = b;
}

bool ClickedField::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::FocusIn) {
        rb->setChecked( true );
        return false;
    }
    return QObject::eventFilter(o, e);
}

ScanWidget::ScanWidget( QWidget *parent )
    : QWidget( parent )
{
    setupUi(this);

    deviceRadioButton->setChecked( true );
    devices = new QMap<QString, QString>;

    deviceComboBox->installEventFilter( new ClickedField(deviceRadioButton, this) );
    folderLineEdit->installEventFilter( new ClickedField(folderRadioButton, this) );
    folderPushButton->installEventFilter( new ClickedField(folderRadioButton, this) );

    progress->setMaximum( 1 );

    updateVolumeList("bar");

    connect( scanButton, SIGNAL( clicked() ), this, SLOT( goScan() ) );
    connect( folderPushButton, SIGNAL( clicked() ), this, SLOT( setFolder() ) );
}

QMap<QString, QString> *ScanWidget::volumeList()
{
    if( devices->count() > 0 )
    {
        return devices;
    }

    if( !QDBusConnection::systemBus().isConnected() ) {
        qDebug("Cannot connect to D-BUS session bus.");
        return devices;
    }

    QDBusInterface hal( HAL_SERVICE, HAL_PATH_MANAGER , HAL_OBJECT_MANAGER, QDBusConnection::systemBus() );
    if (!hal.isValid() ) {
        qDebug( "Couldn't find HAL. Is HAL running?" );
        return devices;
    }

    QDBusReply<QStringList> reply = hal.call( "FindDeviceStringMatch", "volume.disc.type", "dvd_rom" );
    if( !reply.isValid() ) {
        qDebug( "Couldn't call FindDeviceStringMatch." );
        return devices;
    }

    if( reply.value().count() > 0 ) {
        foreach ( QString udi, reply.value() ) {
            QDBusInterface halDev( HAL_SERVICE, udi, HAL_OBJECT_DEVICE, QDBusConnection::systemBus() );
            QDBusReply<bool> isVideo = halDev.call( "GetProperty", "volume.disc.is_videodvd" );
            if( !isVideo.value() ) {
                qDebug("This is a DVD, but not video");
                continue;
            }
            QDBusReply<QString> d = halDev.call( "GetProperty", "block.device" );
            QDBusReply<QString> v = halDev.call( "GetProperty", "volume.label" );
            QRegExp rx("_S(\\d+)_D(\\d+)");
            QString label = v.value();
            rx.indexIn(label);
            if (rx.numCaptures() > 0) {
                label = label.replace(rx, QString(" Season %1 Disc %2").arg(rx.cap(1)).arg(rx.cap(2)));
            } else {
                label = label.replace("_", " ");
            }
            devices->insert( d.value(), label );
        }
    }
    else
    {
        devices->insert( "none detected", "Insert a DVD..." );
    }

    return devices;
}

void ScanWidget::updateVolumeList(QString)
{
    if( devices->count() > 0 )
    {
        devices->clear();
        deviceComboBox->clear();
    }
    QStringList d;
    QMapIterator<QString, QString> i( *volumeList() );
    while (i.hasNext()) {
        i.next();
        d << QString("%1 (%2)").arg(i.value()).arg(i.key());
    }
    deviceComboBox->addItems(d);
}

void ScanWidget::goScan()
{
    scanButton->setDisabled( true );

    if( deviceRadioButton->isChecked() )
    {
        QRegExp rx(".*\\((.*)\\)");
        rx.indexIn(deviceComboBox->currentText());
        emit scanDVD( rx.cap(1) );
    }
    else if( folderRadioButton->isChecked() )
    {
        emit scanDVD( folderLineEdit->text() );
    }
}

void ScanWidget::setProgress( int value, int maximum )
{
    if( progress->maximum() == 1 )
    {
        progress->setMaximum( maximum );
    }

    progress->setValue( value );

    if( value == maximum )
    {
        emit scanningDone();
    }
}

void ScanWidget::setFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory();
    folderLineEdit->setText( folderPath );
}

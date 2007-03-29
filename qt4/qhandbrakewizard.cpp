#include <QtGui>
#include <QtDBus>

#include "qhandbrakewizard.h"
#include "qhandbrake.h"
#include "faderwidget.h"
#include "scanwidget.h"
#include "encodewidget.h"


#define HAL_SERVICE "org.freedesktop.Hal"
#define HAL_PATH_MANAGER "/org/freedesktop/Hal/Manager"
#define HAL_PATH_DEVICE "/org/freedesktop/Hal/Device"
#define HAL_OBJECT_MANAGER "org.freedesktop.Hal.Manager"
#define HAL_OBJECT_DEVICE "org.freedesktop.Hal.Device"


Magic::Magic(QWidget *w, QHandBrakeWizard *q, QObject *p)
    : QObject(p)
{
    m = w;
    qm = q;
}

bool Magic::eventFilter(QObject *o, QEvent *e)
{
    if( e->type() == QEvent::Show ) {
        qm->fadeInWidget( m );
        return false;
    }
    return QObject::eventFilter(o, e);
}

void QHandBrakeWizard::fadeInWidget(QWidget *w)
{
    if( faderWidget ) { faderWidget->close(); }
    faderWidget = new FaderWidget( w );
    faderWidget->start();
}

QHandBrakeWizard::QHandBrakeWizard(QStackedWidget *parent)
    : QStackedWidget(parent)
{
    qhb = new QHandBrake(this);

    scanWidget = new ScanWidget;
    encodeWidget = new EncodeWidget;

    scanWidget->setObjectName("scanWidget");
    encodeWidget->setObjectName("encodeWidget");

    scanWidget->installEventFilter(new Magic(scanWidget, this, this));
    encodeWidget->installEventFilter(new Magic(encodeWidget, this, this));

    addWidget(scanWidget);

    connect(scanWidget, SIGNAL(scanDVD(const QString)), qhb, SLOT(startScan(const QString)));
    connect(scanWidget, SIGNAL(scanningDone()), SLOT(activateEncodePage()));
    connect(qhb, SIGNAL(scanProgress(int, int)), scanWidget, SLOT(setProgress(int, int)));
    connect(encodeWidget, SIGNAL(convert()), qhb, SLOT(encode()));

    if( QDBusConnection::systemBus().isConnected() )
    {
        QDBusConnection::systemBus().connect( HAL_SERVICE, HAL_PATH_MANAGER, HAL_OBJECT_MANAGER, "DeviceAdded", scanWidget, SLOT( updateVolumeList( QString ) ) );
        QDBusConnection::systemBus().connect( HAL_SERVICE, HAL_PATH_MANAGER, HAL_OBJECT_MANAGER, "DeviceRemoved", scanWidget, SLOT( updateVolumeList( QString ) ) );
    }
}

void QHandBrakeWizard::activateScanPage()
{
    setCurrentWidget(scanWidget);
}

void QHandBrakeWizard::activateEncodePage()
{
    addWidget(encodeWidget);

    encodeWidget->setModel( qhb->titleListModel() );

    setCurrentWidget(encodeWidget);
}

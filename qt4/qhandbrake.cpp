#include "qhandbrake.h"

QHandBrake::QHandBrake( QObject *parent ) : QObject( parent )
{
    hbHandle = hb_init_express( HB_DEBUG_NONE, 1 );
    hbTitle = NULL;
}

QHandBrake::~QHandBrake()
{
    hb_close( &hbHandle );
}

QString displayDuration( const QTime t )
{
    return QString( "%1 %2, %3 %4, %5 %6").arg(t.hour()).arg(t.hour() == 1 ? "hour" : "hours").arg(t.minute()).arg(t.minute() == 1 ? "minute" : "minutes").arg(t.second()).arg(t.second() == 1 ? "second" : "seconds");
}

QStandardItemModel *QHandBrake::titleListModel()
{
    hb_list_t *hbTitles;
    int i, titleCount;

    hbTitles = hb_get_titles( hbHandle );
    titleCount = hb_list_count( hbTitles );
    titleModel = new QStandardItemModel( titleCount, 2, this );

    titleModel->setHorizontalHeaderItem( 0, new QStandardItem("Title") );
    titleModel->setHorizontalHeaderItem( 1, new QStandardItem("Duration") );

    for( i = 0; i < titleCount; ++i )
    {
        hb_title_t *t = ( hb_title_t * )hb_list_item( hbTitles, i );

        QTime d = QTime( t->hours, t->minutes, t->seconds );
        QString s = QString( "Title %1" ).arg( t->index );
        QStandardItem *siTitle = new QStandardItem( s );
        QStandardItem *siDuration = new QStandardItem( displayDuration( d ) );
        titleModel->setItem( i, 0, siTitle );
        titleModel->setItem( i, 1, siDuration );

        siTitle->setCheckable( true );
        siTitle->setEditable( false );
        siTitle->setData( i, Qt::UserRole );

        siDuration->setEditable( false );
    }

    return titleModel;
}

void QHandBrake::startScan( const QString sDevice )
{
    hb_state_t s;
    int titleCurrent = 0;
    int titleCount = 0;

    if( sDevice.isEmpty() )
    {
        qDebug("Passed an empty device/path");
        return;
    }

    hb_scan( hbHandle, sDevice.toLocal8Bit(), 0 );

    do {
        hb_get_state( hbHandle, &s );
        qApp->processEvents();
        if( s.state == HB_STATE_SCANNING )
        {
            titleCurrent = s.param.scanning.title_cur;
            titleCount = s.param.scanning.title_count;

            if( titleCurrent > 0 )
            {
                emit scanProgress( titleCurrent, titleCount );
                qApp->processEvents();
            }
        }
    } while( s.state != HB_STATE_SCANDONE );

    if( hb_list_count( hb_get_titles( hbHandle ) ) )
    {
        emit scanProgress( titleCount, titleCount );
    }
    else
    {
        qDebug("Scanning failed");
        return;
    }
}

void QHandBrake::encode()
{
    if( hbHandle == NULL )
    {
        qDebug("Encode called too early");
        return;
    }

    int i;
    hb_list_t *titles = hb_get_titles( hbHandle );

    for( i = 0; i < titleModel->rowCount(); ++i )
    {
        QStandardItem *si = titleModel->item( i, 0 );
        if( si->checkState() == Qt::Checked )
        {
            hbTitle = ( hb_title_t * )hb_list_item( titles, si->data( Qt::UserRole ).toInt() );
            hb_job_t *job = hbTitle->job;
            // FIXME hardcoded params here! need to fix
            job->pixel_ratio = 1;
            job->vcodec = HB_VCODEC_FFMPEG;
            job->vquality = -1.0;
            job->vbitrate = 1600;
            job->acodec = HB_ACODEC_LAME;
            job->audios[0] = -1;
            job->mux = HB_MUX_MP4;
            job->subtitle = -1;
            job->pass = 0;
            job->file = strdup("/tmp/foo.mp4");
            hb_add( hbHandle, job );
        }
    }

    fprintf(stderr, "Calling hb_start...\n");
    hb_start( hbHandle );
}

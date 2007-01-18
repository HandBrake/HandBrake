/*****************************************************************************
 * wxHB:
 *****************************************************************************
 * Copyright (C)
 * $Id: wxHB.cpp,v 1.8 2005/03/26 23:04:17 titer Exp $
 *
 * Authors:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#include "wxHB.h"
#include "hbWizard.h"

#ifdef SYS_CYGWIN
#   include <windows.h>
#endif

/****************************************************************************
 * Definitions / Variables
 ****************************************************************************/
class hbApp *g_hbApp = NULL;

IMPLEMENT_APP( hbApp )

/****************************************************************************
 * Helpers class
 ****************************************************************************/
class hbAppTimer: public wxTimer
{
public:
    hbAppTimer()
    {
        Start( 50, wxTIMER_CONTINUOUS );
    }
    virtual void Notify()
    {
        g_hbApp->Update();
    }
};

class hbAppProgress: public wxDialog
{
public:
    hbAppProgress( wxWindow *parent, wxString title, wxString msg ) :
        wxDialog( parent, -1, title, wxDefaultPosition, wxSize( 250, 300),
                  wxDEFAULT_DIALOG_STYLE )
    {
        /* Create widgets */
        txt = new wxStaticText( this, -1, msg );
        gauge = new wxGauge( this, -1, 100 );
        button = new wxButton( this, wxID_CANCEL, wxU("Cancel") );

        /* Add evrything in a sizer */
        wxBoxSizer *sizer = new wxBoxSizer( wxVERTICAL );
        sizer->Add( txt, 1, wxEXPAND|wxALIGN_CENTER|wxALL, 5 );

        wxBoxSizer *sizerH = new wxBoxSizer( wxHORIZONTAL );
        sizerH->Add( gauge, 1, wxALIGN_CENTER|wxALL, 5 );
        sizer->Add( sizerH, 0, wxALIGN_CENTER|wxALL, 5 );

        sizer->Add( button, 0, wxALIGN_CENTER_HORIZONTAL|
                               wxALIGN_BOTTOM|wxALL, 5 );
        //SetSizerAndFit( sizer );
        SetSizer( sizer );
    }

    void SetProgress( int i_percent, wxString msg )
    {
        gauge->SetValue( i_percent );
        txt->SetLabel( msg );
    }

    void OnClose( wxCloseEvent &event )
    {
        EndModal( wxID_CANCEL );
    }

private:
    wxStaticText *txt;
    wxGauge      *gauge;
    wxButton     *button;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( hbAppProgress, wxDialog )
    EVT_CLOSE( hbAppProgress::OnClose )
END_EVENT_TABLE()

/****************************************************************************
 * hbApp class
 ****************************************************************************/

/* OnInit: Call at the very start and put every in place
 */
bool hbApp::OnInit()
{
    /* Export hbApp */
    g_hbApp = this;


    /* Init all variables */
    isEnd = false;

    progress = NULL;

    i_title = -1;
    titles  = NULL;
    title = NULL;

    audios = NULL;
    subs = NULL;

    systemDrive = NULL;

    /* Start HB */
    if( Init() )
        return false;

    /* Start out timer */
    timer = new hbAppTimer();

    /* Create and Start the wizard */
    wizard = new hbWizard( NULL );
    wizard->Run();

    /* Special hack FIXME */
    isEnd = true;

    return true;
}

/* OnExit:
 */
int hbApp::OnExit()
{
    delete timer;
    delete wizard;

    /* End hb */
    End();

    /* Delete others FIXME */

    return 0;
}

/* Init:
 */
int hbApp::Init()
{
    /* Create a hb instance */
    hbHandle = hb_init( HB_DEBUG_NONE, 1 );
    if( hbHandle == NULL )
        return -1;

    return 0;
}
/* Scan:
 */
int hbApp::Scan( wxString sDevice )
{
    if( sDevice.IsEmpty() )
        return -1;

    /* Reset state */
    i_title = -1;
    if( titles ) delete titles; titles  = NULL;
    if( audios ) delete audios; audios = NULL;
    if( subs ) delete subs; subs = NULL;
    title = NULL;

    /* Append a \ if needed */
    if( sDevice.Length() == 2 && sDevice.Last() == ':' )
        sDevice.Append( '\\' );

    /* Launch the scan (all titles) */
    hb_scan( hbHandle, sDevice.ToAscii(), 0 );

    /* Create a progress report */
    progress = new hbAppProgress( wizard, wxU("Scanning..."), wxU("Opening ")+sDevice);
    progress->ShowModal();

    if( !hb_list_count( hb_get_titles( hbHandle ) ) )
    {
        hbError( wizard, wxU("Scanning failed") );
        return -1;
    }

    /* FIXME test if the scan is complete */


    return 0;
}
/* Encode:
 */
int hbApp::Encode()
{
    hb_state_t s;

    /* Maybe check a few things like:
     *  - compatible muxer and codecs */

    /* Start the encoding */
    hb_add( hbHandle, title->job );
    hb_start( hbHandle );

    /* FIXME use the wizard instead */
    /* Create a progress report */
    progress = new hbAppProgress( wizard, wxU("Encoding..."), wxU("0%"));
    progress->ShowModal();

    hb_get_state( hbHandle, &s );
    if( s.param.workdone.error != HB_ERROR_NONE )
    {
        hb_stop( hbHandle );    /* FIXME to a proper handling */
        hbError( wizard, wxU("Encoding failed/stopped") );
        return -1;
    }

    /* bad */
    return 0;
}

/* End:
 */
void hbApp::End()
{
    hb_close( &hbHandle );
}


wxArrayString *hbApp::GetTitles()
{
    hb_list_t *list;
    int i_count;
    int i;

    if( titles )
        return titles;

    /* Create the title list */
    list = hb_get_titles( hbHandle );

    i_count = hb_list_count( list );
    if( i_count <= 0 )
        return NULL;

    titles = new wxArrayString();
    for( i = 0; i < i_count; i++ )
    {
        hb_title_t *t = (hb_title_t*)hb_list_item( list, i );
        wxString name = wxString::Format( wxU("%d  - %d:%02d:%02d"),
                                          t->index,
                                          t->hours, t->minutes, t->seconds );
        titles->Add( name );
    }

    return titles;
}

void hbApp::SetTitle( int _i_title )
{
    int i;

    if( i_title == _i_title )
        return;

    i_title = _i_title;
    title = (hb_title_t*)hb_list_item( hb_get_titles( hbHandle ), i_title );
    if( audios ) delete audios; audios = NULL;
    if( subs ) delete subs; subs = NULL;

    for( i = 0; i < 8; i++ )
        title->job->audios[i] = -1;
    title->job->subtitle = -1;
}

int hbApp::GetDefaultTitle()
{
    hb_list_t *list;
    int i_best = -1;
    int i_best_length = 0;
    int i;

    list = hb_get_titles( hbHandle );
    for( i = 0; i < hb_list_count( list ); i++ )
    {
        hb_title_t *t = (hb_title_t*)hb_list_item( list, i );
        int i_length = t->hours * 3600 + t->minutes*60 + t->seconds;

        if( i_best < 0 || i_length > i_best_length )
        {
            i_best = i;
            i_best_length = i_length;
        }
    }

    return i_best;
}



int hbApp::GetChaptersCount()
{
    if( i_title < 0 )
        return 0;

    return hb_list_count( title->list_chapter );
}

void hbApp::SetChapterStart( int i_chapter )
{
    title->job->chapter_start = i_chapter;
}

void hbApp::SetChapterEnd( int i_chapter )
{
    title->job->chapter_end = i_chapter;
}

wxArrayString *hbApp::GetTracksAudio()
{
    int i;
    if( audios )
        return audios;

    audios = new wxArrayString();
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        hb_audio_t *a = (hb_audio_t*)hb_list_item( title->list_audio, i );

        audios->Add( wxU(a->lang) );
    }
    audios->Add( wxU("None") );

    return audios;
}
void hbApp::SetTrackAudio( int i_track, int i_pos )
{
    if( i_pos >= 0 && i_pos < hb_list_count( title->list_audio ) )
        title->job->audios[i_pos] = i_track;
    else
        title->job->audios[i_pos] = -1;
}

wxArrayString *hbApp::GetTracksSubtitle()
{
    int i;

    if( subs )
        return subs;

    subs = new wxArrayString();
    for( i = 0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        hb_subtitle_t *s =
            (hb_subtitle_t*)hb_list_item( title->list_subtitle, i );

        subs->Add( wxU(s->lang) );
    }
    subs->Add( wxU("None") );

    return subs;
}

void hbApp::SetTrackSubtitle( int i_track )
{
    if( i_track >= 0 && i_track < hb_list_count( title->list_subtitle ) )
        title->job->subtitle = i_track;
    else
        title->job->subtitle = -1;
}

void hbApp::SetOutputFile( wxString sFile )
{
    char *psz;

    title->job->file = strdup( sFile.ToAscii() ); /* FIXME do we need strdup ? */

    psz = strrchr( title->job->file, '.' );
    if( psz == NULL )
        title->job->mux = HB_MUX_AVI; /* By default, FIXME */
    else if( !strcasecmp( psz, ".avi" ) )
        title->job->mux = HB_MUX_AVI;
    else if( !strcasecmp( psz, ".mp4" ) || !strcasecmp( psz, ".mov") )
        title->job->mux = HB_MUX_MP4;
    else if( !strcasecmp( psz, ".ogg" ) || !strcasecmp( psz, ".ogm") )
        title->job->mux = HB_MUX_OGM;
    else
        title->job->mux = HB_MUX_AVI; /* By default */

    /* Fix acodec value */
    switch( title->job->mux )
    {
        case HB_MUX_AVI:
            title->job->acodec = HB_ACODEC_LAME;
            break;
        case HB_MUX_MP4:
            title->job->acodec = HB_ACODEC_FAAC;
            break;
        case HB_MUX_OGM:
            title->job->acodec = HB_ACODEC_VORBIS;
            break;
    }
}

void hbApp::SetAudioBitrate( int i_bitrate )
{
    int i_old = title->job->abitrate;

    title->job->abitrate = i_bitrate;

    if( title->job->vbitrate > 0 )
        title->job->vbitrate = title->job->vbitrate + (i_old - i_bitrate);
}

void hbApp::SetTotalBitrate( int i_bitrate )
{
    title->job->vbitrate = i_bitrate - title->job->abitrate;

    if( title->job->vbitrate <= 0 )
        title->job->vbitrate = 1;
}

void hbApp::SetTotalSize( int i_size )
{
    /* XXX */
}

void hbApp::SetVideoPass( int i_pass )
{
    /* FIXME is 0 or 1 valid for 1 pass ? */
    if( i_pass <= 1 )
        title->job->pass = 0;
    else if( i_pass == 2 )
        title->job->pass = 2;
}

void hbApp::SetVideoCodec( wxString sCodec )
{
    if( sCodec.Lower() == wxU("ffmpeg") )
        title->job->vcodec = HB_VCODEC_FFMPEG;
    else if( sCodec.Lower() == wxU("xvid") )
        title->job->vcodec = HB_VCODEC_XVID;
    else if( sCodec.Lower() == wxU("x264") )
        title->job->vcodec = HB_VCODEC_XVID;
}

void hbApp::SetVideoDeinterlace( bool b_deinterlace )
{
    title->job->deinterlace = b_deinterlace ? 1 : 0;
}

void hbApp::SetPriority( int i_priority )
{
    /* Doesn't work :(( */
#ifdef SYS_CYGWIN
    switch( i_priority )
    {
        case HB_PRIORITY_ABOVE_NORMAL:
            i_priority = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        case HB_PRIORITY_BELOW_NORMAL:
            i_priority = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        case HB_PRIORITY_LOWEST:
            i_priority = THREAD_PRIORITY_LOWEST;
            break;
        case HB_PRIORITY_HIGHEST:
            i_priority = THREAD_PRIORITY_HIGHEST;
            break;

        case HB_PRIORITY_NORMAL:
        default:
            i_priority = THREAD_PRIORITY_NORMAL;
            break;
    }
    ::SetThreadPriority( GetCurrentThread(), i_priority );
#else
    /* TODO */
#endif
}

void hbApp::SetCpuCount( int i_cpu )
{
    hb_set_cpu_count( hbHandle, i_cpu );
}

int hbApp::GetVideoRateBase()
{
    return title->rate_base;
}

void hbApp::SetVideoRateBase( int i_base )
{
    title->job->vrate_base = i_base;
    title->job->vrate = HB_VIDEO_RATE_BASE;
}
void hbApp::GetVideoAutocrop( int crop[4] )
{
    int i;
    for( i = 0; i < 4; i++ )
        crop[i] = title->crop[i];
}
void hbApp::SetVideoCrop( int crop[4] )
{
    int i;
    for( i = 0; i < 4; i++ )
        title->job->crop[i] = crop ? crop[i] : 0;
}
void hbApp::GetVideoSize( int *pi_width, int *pi_height )
{
    *pi_width = title->width;
    *pi_height = title->height;
}
void hbApp::SetVideoSize( int i_width, int i_height )
{
    title->job->width = i_width;
    title->job->height = i_height;
}


int hbApp::GetAudioSamplerate()
{
    int idx = title->job->audios[0];
    if( idx >= 0 && idx < hb_list_count( title->list_audio ) )
    {
        hb_audio_t *a = (hb_audio_t*)hb_list_item( title->list_audio, 0 );
        return a->rate;
    }

    /* FIXME not good */
    return title->job->arate;
}

void hbApp::SetAudioSamplerate( int i_rate )
{
    title->job->arate = i_rate;
}

wxString hbApp::GetDefaultAudio()
{
    return wxU("English");
}

wxString hbApp::GetDefaultSubtitle()
{
    return wxU("None");
}

int hbApp::GetDefaultCpuCount()
{
    return GetSystemCpuCount();
}

int hbApp::GetDefaultPriority()
{
    return HB_PRIORITY_NORMAL;
}

int hbApp::GetDefaultVideoRateBase()
{
    return 0;
}

int hbApp::GetDefaultAudioSamplerate()
{
    return 0;
}

/* Drive */
wxArrayString *hbApp::GetSystemDrive()
{
    if( systemDrive )
        return systemDrive;

    systemDrive = new wxArrayString();
#ifdef SYS_CYGWIN
    char c;
    for( c = 'A'; c <= 'Z'; c++ )
    {
        char pszDrive[4];
        pszDrive[0] = c;
        pszDrive[1] = ':';
        pszDrive[2] = '\\';
        pszDrive[3] = '\0';

        if( GetDriveType( pszDrive ) == DRIVE_CDROM )
            systemDrive->Add( wxString::Format( wxU("%c:"), c ) );
    }
#else
    /* TODO true detection */
    systemDrive->Add( wxU("/dev/dvd") );
    systemDrive->Add( wxU("/dev/cdrom") );
#endif

    return systemDrive;
}

int hbApp::GetSystemCpuCount()
{
    return hb_get_cpu_count();
}

void hbApp::Update()
{
    hb_state_t s;

    /* Special hack FIXME */
    if( isEnd )
    {
        g_hbApp->ExitMainLoop();
        return;
    }

    /* */
    hb_get_state( hbHandle, &s );
    switch( s.state )
    {
        case HB_STATE_IDLE:
            break;

        case HB_STATE_SCANNING:
        {
            int i_cur = s.param.scanning.title_cur;
            int i_cnt = s.param.scanning.title_count;

            if( i_cnt > 0 )
            {
                progress->SetProgress( 100*(i_cur-1)/i_cnt,
                                   wxString::Format(wxU("Scanning title %d of %d."),
                                                    i_cur, i_cnt) );
            }
            break;
        }

        case HB_STATE_SCANDONE:
            if( progress )
            {
                progress->SetProgress( 100, wxU("Scanning complete.") );
                progress->Close( TRUE );
                //delete progress;
                progress = NULL;
            }
            break;

        case HB_STATE_WORKING:
        {
            float f_progress = s.param.working.progress;
            float f_rate_cur = s.param.working.rate_cur;
            float f_rate_avg = s.param.working.rate_avg;

            progress->SetProgress( (int)(100 * f_progress),
                wxString::Format(wxU("Encoding: %.2f %% (%.2f fps, avg %.2f fps)\n"),
                                 100.0 * f_progress, f_rate_cur, f_rate_avg ));
            break;
        }

        case HB_STATE_WORKDONE:
            if( progress )
            {
                progress->SetProgress( 100, wxU("Encoding complete.") );
                progress->Close( TRUE );
                //delete progress;
                progress = NULL;
            }
            break;
    }
}


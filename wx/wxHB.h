/*****************************************************************************
 * wxHB:
 *****************************************************************************
 * Copyright (C) 
 * $Id: wxHB.h,v 1.2 2005/01/16 17:44:56 titer Exp $
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

#include "hb.h"

#include <wx/wx.h>
#include <wx/wizard.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/dnd.h>
#include <wx/treectrl.h>
#include <wx/gauge.h>
#include <wx/accel.h>
#include <wx/checkbox.h>
#include <wx/image.h>
#include <wx/wizard.h>

#include "wx/file.h"
#include "wx/mstream.h"
#include "wx/wfstream.h"
#include <wx/statline.h>

#if wxUSE_UNICODE
#define wxU(psz) wxString(psz, *wxConvCurrent)
#else
#define wxU(psz) psz
#endif

/* Unique global variable: our App
 * XXX: we could use wxGetApp() instead (but it is a bit longer ;)
 */
extern class hbApp *g_hbApp;

/* */
class hbAppTimer;
class hbAppProgress;
class hbWizard;

/* */
enum
{
    HB_PRIORITY_LOWEST = 0,
    HB_PRIORITY_BELOW_NORMAL,
    HB_PRIORITY_NORMAL,
    HB_PRIORITY_ABOVE_NORMAL,
    HB_PRIORITY_HIGHEST,
};

/* Our wxApp */
class hbApp: public wxApp
{
public:
    virtual bool OnInit();
    virtual int  OnExit();

    /* HB related */
    int  Init();
    int  Scan( wxString sDevice );
    int  Encode();
    void End();

    /* */
    /* XXX: the wxArrayString aren't duplicated, so don't delete them */
    wxArrayString *GetTitles();
    void           SetTitle( int i_title );

    int            GetChaptersCount();
    void           SetChapterStart( int i_chapter );
    void           SetChapterEnd( int i_chapter );

    wxArrayString *GetTracksAudio();
    void           SetTrackAudio( int i_track, int i_pos = 0 );

    wxArrayString *GetTracksSubtitle();
    void           SetTrackSubtitle( int i_track );

    void           SetOutputFile( wxString );
    void           SetAudioBitrate( int );
    void           SetTotalBitrate( int );
    void           SetTotalSize( int );
    void           SetVideoPass( int );
    void           SetPriority( int );
    void           SetVideoCodec( wxString );
    void           SetVideoDeinterlace( bool );
    void           SetCpuCount( int );

    int            GetVideoRateBase();
    void           SetVideoRateBase( int );

    int            GetAudioSamplerate();
    void           SetAudioSamplerate( int );

    void           GetVideoAutocrop( int crop[4] );
    void           SetVideoCrop( int crop[4] );
    void           GetVideoSize( int *pi_width, int *pi_height );
    void           SetVideoSize( int i_width, int i_height );


    /* Get Default value (from config file or from logic) */
    int            GetDefaultTitle();
    wxString       GetDefaultAudio();
    wxString       GetDefaultSubtitle();
    int            GetDefaultCpuCount();
    int            GetDefaultPriority();
    int            GetDefaultVideoRateBase();
    int            GetDefaultAudioSamplerate();

    /* Drive */
    wxArrayString *GetSystemDrive();
    int            GetSystemCpuCount();


    /* wx related */
    /* Called regulary by a timer
     * FIXME derive hbApp from timer too instead ? */
    void  Update();

private:
    bool isEnd;

    hbAppTimer *timer;
    hbWizard *wizard;
    hbAppProgress *progress;

    hb_handle_t *hbHandle;

    /* */
    int           i_title;
    wxArrayString *titles;
    hb_title_t    *title;

    int            i_chapters;
    wxArrayString *audios;
    wxArrayString *subs;

    /* System */
    wxArrayString *systemDrive;
};

static void hbError( wxWindow *p_parent, wxString sTitle )
{
    wxMessageDialog *dlg = new wxMessageDialog( p_parent,
                                                sTitle,
                                                wxU("Error"),
                                                wxOK|wxCENTRE|wxICON_ERROR );
    dlg->ShowModal();
    delete dlg;
}

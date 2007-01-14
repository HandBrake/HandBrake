/*****************************************************************************
 * wxHB:
 *****************************************************************************
 * Copyright (C)
 * $Id: hbWizard.cpp,v 1.4 2005/03/17 19:22:47 titer Exp $
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

#include "hb32x32.xpm"

/* All IDs
 * CB: ComboBox
 * BT: Button
 * RD: Radio
 * NB: NoteBook
 */
enum
{
    /* Source page */
    ID_SOURCE_RD_DRIVE = wxID_HIGHEST,
    ID_SOURCE_RD_DIR,
    ID_SOURCE_BT_DIR,
    ID_SOURCE_CB_DRIVE,
    ID_SOURCE_CB_DIR,

    ID_SOURCE_BT_ABOUT,
    ID_SOURCE_BT_PREFERENCES,

    /* Settings page */
    ID_SETTINGS_CB_TITLE = wxID_HIGHEST,
    ID_SETTINGS_CB_CHAPTER_START,
    ID_SETTINGS_CB_CHAPTER_END,
    ID_SETTINGS_CB_AUDIO_LANGUAGE,
    ID_SETTINGS_CB_AUDIO_QUALITY,
    ID_SETTINGS_CB_SUBTITLE_LANGUAGE,
    ID_SETTINGS_CB_VIDEO_QUALITY,
    ID_SETTINGS_CB_OUTPUT_FILE,
    ID_SETTINGS_CB_OUTPUT_SIZE,
    ID_SETTINGS_CB_OUTPUT_BITRATE,
    ID_SETTINGS_SC_OUTPUT_FILE_COUNT,

    ID_SETTINGS_RD_OUTPUT_SIZE,
    ID_SETTINGS_RD_OUTPUT_BITRATE,

    ID_SETTINGS_CB_VIDEO_CODEC,
    ID_SETTINGS_CB_VIDEO_FPS,
    ID_SETTINGS_CH_VIDEO_DEINTERLACE,
    ID_SETTINGS_CH_VIDEO_CROP,
    ID_SETTINGS_SC_VIDEO_CROP_LEFT,
    ID_SETTINGS_SC_VIDEO_CROP_TOP,
    ID_SETTINGS_SC_VIDEO_CROP_RIGHT,
    ID_SETTINGS_SC_VIDEO_CROP_BOTTOM,
    ID_SETTINGS_SC_VIDEO_WIDTH,
    ID_SETTINGS_SC_VIDEO_HEIGHT,

    ID_SETTINGS_CB_AUDIO_SAMPLERATE,

    ID_SETTINGS_CB_CPU,
    ID_SETTINGS_CB_PRIORITY,

    ID_SETTINGS_BT_FILE,

    ID_SETTINGS_NB,

    /* */
    //ID_ALL_FIXME,
};

class hbImage: public wxControl
{
public:
    hbImage( wxWindow *parent ) :
        wxControl( parent, -1, wxDefaultPosition, wxSize(128,128),
                   wxBORDER_NONE  )
    {
    }

    void OnPaint(wxPaintEvent &event)
    {
        wxPaintDC dc(this);
        dc.SetBrush( wxBrush(*wxGREEN, wxSOLID) );
        dc.DrawText( wxU("Solid green"), 10, 10 );
    }

private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( hbImage, wxControl )
    EVT_PAINT( hbImage::OnPaint )
END_EVENT_TABLE()

/*****************************************************************************
 * hbWizard
 *****************************************************************************/
static void hbWizardPageHeaders( wxWizardPageSimple *page,
                                 char *psz_title,
                                 char *psz_description,
                                 wxSizer *sizerC )
{
    wxBoxSizer *sizerH = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBitmap *bm = new wxStaticBitmap( page, wxID_ANY,
                                             wxBitmap(hb_xpm) );
    wxBoxSizer *sizerV = new wxBoxSizer(wxVERTICAL);


    wxStaticText *title = new wxStaticText( page, -1, wxU(psz_title) );
    wxFont font = title->GetFont();
    font.SetPointSize(14);
    title->SetFont( font );

    sizerV->Add( title, 0, wxALIGN_CENTER|wxALL, 5 );

    sizerV->Add( new wxStaticLine(page, -1), 0, wxEXPAND|wxALL, 5 );

    wxStaticText *text =  new wxStaticText( page, -1,
                                            wxU( psz_description ) );
    sizerV->Add( text, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

    //sizerV->Add( 0, 1, 1, wxALL );

    sizerV->Add( sizerC, 1, wxEXPAND|wxALIGN_CENTER|wxALL, 5 );


    /* Add bmp */
    sizerH->Add( bm, 0, wxALL|wxALIGN_CENTER, 5 );
    /* Add sizer */
    sizerH->Add( sizerV, 1, wxEXPAND|wxALIGN_CENTER|wxALL, 5 );

    /* */
    page->SetSizerAndFit( sizerH );

    /* Needed ? */
    page->SetAutoLayout( TRUE );
    page->Layout();
}

/*****************************************************************************
 * hbWizardPageSource:
 *****************************************************************************/
class hbWizardPageSource: public wxWizardPageSimple
{
public:
    hbWizardPageSource( wxWizard *p_wizard );
    virtual ~hbWizardPageSource();


    void OnRadio( wxCommandEvent &event );
    void OnBrowseDir( wxCommandEvent &event );
    void OnTextDirChange( wxCommandEvent &event );
    void OnTextDriveChange( wxCommandEvent &event );
    void OnPageChanging( wxWizardEvent &event );
    
private:

    wxDirDialog *dlgDir;
    wxComboBox *comboDir;
    wxComboBox *comboDrive;
    wxButton   *buttonDir;

    DECLARE_EVENT_TABLE()
};


hbWizardPageSource::hbWizardPageSource( wxWizard *p_wizard ) :
                                                 wxWizardPageSimple(p_wizard)
{
    wxRadioButton *radio[2];
    wxFlexGridSizer *sizerDrive;
    wxFlexGridSizer *sizerDir;
    wxBoxSizer *sizerH;
    wxButton *button;
    
    /* Create radio and subpanel */
    radio[0] = new wxRadioButton( this, ID_SOURCE_RD_DRIVE,
                                  wxU("DVD drive" ) );
    radio[1] = new wxRadioButton( this, ID_SOURCE_RD_DIR,
                                  wxU("DVD directory" ) );

    /* - Create DRIVE row */
    sizerDrive = new wxFlexGridSizer( 1, 1, 20 );
    /* combo */
    wxArrayString &sD = *g_hbApp->GetSystemDrive();
    comboDrive = new wxComboBox( this, ID_SOURCE_CB_DRIVE, sD[0],
                                 wxDefaultPosition, wxDefaultSize,
                                 sD );
    sizerDrive->Add( comboDrive, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );

    /* - Create DIR row */
    sizerDir = new wxFlexGridSizer( 2, 1, 20 );
    /* combo */
    comboDir = new wxComboBox( this, ID_SOURCE_CB_DIR,  wxU("") );
    sizerDir->Add( comboDir, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
    /* button */
    buttonDir = new wxButton( this, ID_SOURCE_BT_DIR, wxU("Browse..."));
    sizerDir->Add( buttonDir,
                   0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );

    /* - Hop in the main */
    wxFlexGridSizer *sizerChoice = new wxFlexGridSizer( 2, 2, 20 );
    sizerChoice->Add( radio[0],
                      0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
    sizerChoice->Add( sizerDrive, 1, wxEXPAND|wxALIGN_LEFT|
                                     wxALIGN_CENTER_VERTICAL|wxALL, 5  );
    sizerChoice->Add( radio[1],
                      0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
    sizerChoice->Add( sizerDir, 1, wxEXPAND|wxALIGN_LEFT|
                                   wxALIGN_CENTER_VERTICAL|wxALL, 5  );

    wxBoxSizer *sizer = new wxBoxSizer( wxVERTICAL );
    sizer->Add( sizerChoice, 1,
                wxEXPAND|wxALIGN_CENTER|wxALL, 5 );

    sizer->Add( 0, 0 );

    sizerH = new wxBoxSizer( wxHORIZONTAL );
    sizerH->Add( 0, 0, 1 );
    button = new wxButton( this, ID_SOURCE_BT_ABOUT, wxU("About") );
    button->Enable(FALSE);
    sizerH->Add( button, 0, wxALL|wxALIGN_RIGHT, 5 );
    button = new wxButton( this, ID_SOURCE_BT_PREFERENCES, wxU("Preferences"));
    button->Enable(FALSE);
    sizerH->Add( button, 0, wxALL|wxALIGN_RIGHT, 5 );

    sizer->Add( sizerH, 0, wxALIGN_BOTTOM|wxEXPAND, 5 );

    /* */
#if 0
    hbImage *img = new hbImage( this );
    sizer->Add( img );
#endif
    

    hbWizardPageHeaders( this, "Source",
                         "Select the DVD (drive or a directory)", sizer );

    comboDir->Enable( FALSE );
    buttonDir->Enable( FALSE );

    //SetAutoLayout( TRUE );
    //Layout();

    /* */
    dlgDir = NULL;
}

hbWizardPageSource::~hbWizardPageSource()
{
}

void hbWizardPageSource::OnRadio( wxCommandEvent &event )
{
    if( event.GetId() == ID_SOURCE_RD_DRIVE )
    {
        comboDrive->Enable( TRUE );

        comboDir->Enable( FALSE );
        buttonDir->Enable( FALSE );
    }
    else
    {
        comboDir->Enable( TRUE );
        buttonDir->Enable( TRUE );

        comboDrive->Enable( FALSE );
    }
}

void hbWizardPageSource::OnBrowseDir( wxCommandEvent &event )
{
    if( dlgDir == NULL )
        dlgDir = new wxDirDialog( this, wxU( "Choose a directory"),
                                      wxGetCwd() );

    if( dlgDir->ShowModal() == wxID_OK )
    {
        const wxChar *p = dlgDir->GetPath();

        comboDir->SetValue( p );
    }
}

void hbWizardPageSource::OnTextDirChange( wxCommandEvent &event )
{
}
void hbWizardPageSource::OnTextDriveChange( wxCommandEvent &event )
{
}

void hbWizardPageSource::OnPageChanging( wxWizardEvent &event )
{
    wxRadioButton *rdDrive = (wxRadioButton*)FindWindow( ID_SOURCE_RD_DRIVE );
    wxString sDevice;

    if( rdDrive->GetValue() )
    {
        wxComboBox *cbDrive = (wxComboBox*)FindWindow( ID_SOURCE_CB_DRIVE );
        /* Get the drive value */
        sDevice = cbDrive->GetValue();
    }
    else
    {
        wxComboBox *cbDir = (wxComboBox*)FindWindow( ID_SOURCE_CB_DIR );
        sDevice = cbDir->GetValue();

        if( sDevice.IsEmpty() )
        {
            hbError( this, wxU("You have to give a directory name") );
            event.Veto();
            return;
        }
    }

    if( g_hbApp->Scan( sDevice ) < 0 )
    {
        event.Veto();
    }
}



BEGIN_EVENT_TABLE( hbWizardPageSource, wxWizardPageSimple )
    EVT_RADIOBUTTON( ID_SOURCE_RD_DRIVE, hbWizardPageSource::OnRadio )
    EVT_RADIOBUTTON( ID_SOURCE_RD_DIR, hbWizardPageSource::OnRadio )

    EVT_BUTTON( ID_SOURCE_BT_DIR, hbWizardPageSource::OnBrowseDir )

    EVT_TEXT( ID_SOURCE_CB_DIR, hbWizardPageSource::OnTextDirChange )
    EVT_TEXT( ID_SOURCE_CB_DRIVE, hbWizardPageSource::OnTextDriveChange )

    EVT_WIZARD_PAGE_CHANGING( -1, hbWizardPageSource::OnPageChanging )
END_EVENT_TABLE()


/*****************************************************************************
 * hbWizardPageSettings:
 *****************************************************************************/
static wxPanel *PanelSettingsGeneral( wxWindow *p_parent )
{
    wxPanel *panel = new wxPanel( p_parent, -1,
                                  wxDefaultPosition, wxSize(200, 200));
    wxBoxSizer *sizerV = new wxBoxSizer( wxVERTICAL );
    wxGridSizer *sizerG;
    wxBoxSizer *sizerS;
    wxStaticBox *ibox;
    wxStaticBoxSizer *sbox;
    wxFlexGridSizer *sizerF;
    wxComboBox  *combo;
    wxButton *button;
    wxRadioButton *radio;
    wxSpinCtrl *spin;

    wxArrayString aQ;

    /* Selection (title/chapter) */
    ibox = new wxStaticBox( panel, -1, wxU("Selection") );
    sbox  = new wxStaticBoxSizer( ibox, wxHORIZONTAL);

    sbox->Add( new wxStaticText( panel, -1, wxU("Title") ),
                 0, wxALL, 5 );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_TITLE, wxU(""),
                            wxDefaultPosition, wxDefaultSize, 0, NULL,
                            wxCB_READONLY );

    sbox->Add( combo, 1, wxEXPAND|wxALL, 5 );
    //sbox->Add( 0, 0, 1 );

    sbox->Add( new wxStaticText( panel, -1, wxU( "Chapters" ) ),
                 0, wxALL, 5 );

    combo = new wxComboBox( panel, ID_SETTINGS_CB_CHAPTER_START, wxU(""),
                            wxDefaultPosition, wxDefaultSize, 0, NULL,
                            wxCB_READONLY );
    sbox->Add( combo, 0, wxEXPAND|wxALL, 5 );

    sbox->Add( new wxStaticText( panel, -1, wxU( "to" ) ),
                 0, wxALL, 5 );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_CHAPTER_END, wxU(""),
                            wxDefaultPosition, wxDefaultSize, 0, NULL,
                            wxCB_READONLY );

    sbox->Add( combo, 0, wxEXPAND|wxALL, 5 );

    sizerV->Add( sbox, 1, wxEXPAND|wxALL, 5 );

    /* Sizer for audio + video box */
    sizerG = new wxGridSizer( 1, 2, 10, 10 );

    /* Audio */
    ibox = new wxStaticBox( panel, -1, wxU("Audio") );
    sbox  = new wxStaticBoxSizer( ibox, wxHORIZONTAL);

    sizerF = new wxFlexGridSizer( 2, 2, 20 );
    sizerF->Add( new wxStaticText( panel, -1, wxU( "Language" ) ),
                 0, wxALL|wxALIGN_LEFT, 5 );
    //sbox->Add( 0, 0, 1 );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_AUDIO_LANGUAGE, wxU(""),
                            wxDefaultPosition, wxDefaultSize, 0, NULL,
                            wxCB_READONLY );
    sizerF->Add( combo, 1, wxEXPAND|wxALL|wxALIGN_RIGHT, 5 );
    
    sizerF->Add( new wxStaticText( panel, -1, wxU( "Quality" ) ),
                 0, wxALL|wxALIGN_LEFT, 5 );
    //sbox->Add( 0, 0, 1 );

    aQ.Clear();
    aQ.Add( wxU("Low - 64 kb/s") );
    aQ.Add( wxU("Normal - 128 kb/s") );
    aQ.Add( wxU("High - 160 kb/s") );
    aQ.Add( wxU("Extra - 192 kb/s") );

    combo = new wxComboBox( panel, ID_SETTINGS_CB_AUDIO_QUALITY, aQ[2],
                            wxDefaultPosition, wxDefaultSize,
                            aQ, wxCB_READONLY );
    sizerF->Add( combo, 1, wxEXPAND|wxALL|wxALIGN_RIGHT, 5 );

    sbox->Add( sizerF, 1, wxEXPAND|wxALL, 0 );
    //sizerV->Add( sbox, 0, wxALL, 5 );
    sizerG->Add( sbox, 1, wxEXPAND|wxALL, 0 );

    /* Video */
    ibox = new wxStaticBox( panel, -1, wxU("Video") );
    sbox  = new wxStaticBoxSizer( ibox, wxHORIZONTAL);

    sizerF = new wxFlexGridSizer( 2, 2, 20 );

    sizerF->Add( new wxStaticText( panel, -1, wxU( "Quality" ) ),
                 0, wxALL|wxALIGN_LEFT, 5 );

    aQ.Clear();
    aQ.Add( wxU("Low (fast)") );
    aQ.Add( wxU("Normal") );
    aQ.Add( wxU("High (slow)") );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_VIDEO_QUALITY, aQ[2],
                            wxDefaultPosition, wxDefaultSize,
                            aQ, wxCB_READONLY );
    sizerF->Add( combo, 1, wxEXPAND|wxALL|wxALIGN_RIGHT, 5 );

    sizerF->Add( new wxStaticText( panel, -1, wxU( "Subtitle" ) ),
                 0, wxALL|wxALIGN_LEFT, 5 );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_SUBTITLE_LANGUAGE, wxU(""),
                            wxDefaultPosition, wxDefaultSize, 0, NULL,
                            wxCB_READONLY );
    sizerF->Add( combo, 1, wxEXPAND|wxALL|wxALIGN_RIGHT, 5 );

    sbox->Add( sizerF, 0, wxALL, 5 );

    sizerG->Add( sbox, 1, wxEXPAND|wxALL, 0 );

    sizerV->Add( sizerG, 1, wxEXPAND|wxALL, 5 );

    /* Output */
    ibox = new wxStaticBox( panel, -1, wxU("Output") );
    sbox  = new wxStaticBoxSizer( ibox, wxVERTICAL);

    sizerF = new wxFlexGridSizer( 3, 1, 20 );
    sizerF->Add( new wxStaticText( panel, -1, wxU("File") ),
                     0, wxALL|wxALIGN_LEFT, 5 );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_OUTPUT_FILE, wxU("") );

    sizerF->Add( combo, 1, wxEXPAND|wxALIGN_RIGHT|
                           wxALIGN_CENTER_VERTICAL|wxALL, 5 );
    button = new wxButton( panel, ID_SETTINGS_BT_FILE, wxU("Browse..."));
    sizerF->Add( button, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
    sbox->Add( sizerF, 0, wxEXPAND|wxALL, 5 );


    sizerS = new wxBoxSizer( wxHORIZONTAL );

    sizerF = new wxFlexGridSizer( 2, 2, 20 );

    aQ.Clear();
    aQ.Add( wxU("800") ); aQ.Add( wxU("700") );
    aQ.Add( wxU("650") ); aQ.Add( wxU("350") );
    aQ.Add( wxU("250") ); aQ.Add( wxU("200") );
    aQ.Add( wxU("150") );
    radio = new wxRadioButton( panel, ID_SETTINGS_RD_OUTPUT_SIZE, wxU("Size") );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_OUTPUT_SIZE, wxU(""),
                            wxDefaultPosition, wxDefaultSize, aQ );
    sizerF->Add( radio, 0, wxALL|wxALIGN_LEFT );
    sizerF->Add( combo, 1, wxEXPAND|wxALL|wxALIGN_RIGHT );
    radio->SetValue( FALSE );
    combo->Enable( FALSE );

    aQ.Clear();
    aQ.Add( wxU("2500") ); aQ.Add( wxU("2000") );
    aQ.Add( wxU("1500") ); aQ.Add( wxU("1000") );
    aQ.Add( wxU("800") ); aQ.Add( wxU("500") );
    aQ.Add( wxU("300") );
    radio = new wxRadioButton( panel, ID_SETTINGS_RD_OUTPUT_BITRATE,
                               wxU("Bitrate" ) );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_OUTPUT_BITRATE, aQ[3],
                            wxDefaultPosition, wxDefaultSize, aQ );
    sizerF->Add( radio, 0, wxALL|wxALIGN_LEFT );
    sizerF->Add( combo, 1, wxEXPAND|wxALL|wxALIGN_RIGHT );
    radio->SetValue( TRUE );
    combo->Enable( TRUE );

    sizerS->Add( sizerF, 0, wxALL|wxALIGN_CENTER_VERTICAL );


    wxBoxSizer *sizerSS = new wxBoxSizer( wxHORIZONTAL );
    sizerSS->Add( new wxStaticText( panel, -1, wxU( "x" ) ), 0, wxALL, 5 );
    spin = new wxSpinCtrl( panel, ID_SETTINGS_SC_OUTPUT_FILE_COUNT, wxU("1"),
                           wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 1, 5, 1 );
    spin->Enable( FALSE );
    sizerSS->Add( spin, 0, wxALL, 5 );

    sizerS->Add( sizerSS, 0, wxALL|wxALIGN_CENTER_VERTICAL );

    sbox->Add( sizerS, 1, wxEXPAND|wxALL, 5 );

    sizerV->Add( sbox, 1, wxEXPAND|wxALL, 5 );

    panel->SetSizerAndFit( sizerV );

    return panel;
}

static wxPanel *PanelSettingsVideo( wxWindow *p_parent )
{
    wxPanel *panel = new wxPanel( p_parent, -1,
                                  wxDefaultPosition, wxSize(200, 200));
    wxFlexGridSizer *sizer = new wxFlexGridSizer( 2, 5, 20 );
    wxBoxSizer *sizer2;
    wxBoxSizer *sizer3;
    wxArrayString aS;
    wxComboBox *combo;
    wxCheckBox *check;
    wxSpinCtrl *spin;
    int i;

    
    /* Codec */
    sizer->Add( new wxStaticText( panel, -1, wxU("Codec") ),
                1, wxEXPAND|wxALL|wxALIGN_LEFT, 5 );
    aS.Clear();
    aS.Add( wxU("FFmpeg") );
    aS.Add( wxU("XviD") );
    aS.Add( wxU("x264") );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_VIDEO_CODEC, aS[0],
                            wxDefaultPosition, wxDefaultSize,
                            aS, wxCB_READONLY );
    sizer->Add( combo, 0, wxALL|wxALIGN_RIGHT, 5 );

    /* FPS */
    sizer->Add( new wxStaticText( panel, -1, wxU("Fps") ),
                1, wxEXPAND|wxALL|wxALIGN_LEFT, 5 );
    aS.Clear();
    for( i = 0; i < hb_video_rates_count; i++ )
        aS.Add( wxU(hb_video_rates[i].string) );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_VIDEO_FPS, wxU(""),
                            wxDefaultPosition, wxDefaultSize,
                            aS, wxCB_READONLY );
    sizer->Add( combo, 0, wxALL|wxALIGN_RIGHT, 5 );

    
    /* Deinterlace */
    check = new wxCheckBox( panel, ID_SETTINGS_CH_VIDEO_DEINTERLACE,
                            wxU("Deinterlace") );
    check->SetValue( FALSE );

    sizer->Add( check, 1, wxEXPAND|wxALL|wxALIGN_LEFT, 5 );
    sizer->Add( 0, 0, 0 );

    /* Crop */
    check = new wxCheckBox( panel, ID_SETTINGS_CH_VIDEO_CROP, wxU("Crop") );
    check->SetValue( FALSE );
    sizer->Add( check, 0, wxTOP|wxBOTTOM|
                          wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );
#if 0
    sizer2 = new wxBoxSizer( wxVERTICAL );
    radio = new wxRadioButton( panel, ID_SETTINGS_RD_VIDEO_AUTOCROP,
                                  wxU("Autocrop" ) );
    radio->SetValue( TRUE );
    sizer2->Add( radio, 0, wxTOP|wxBOTTOM|
                           wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );
    radio = new wxRadioButton( panel, ID_SETTINGS_RD_VIDEO_CUSTOMCROP,
                                  wxU("Custom crop" ) );
    radio->SetValue( FALSE );
    sizer2->Add( radio, 0, wxTOP|wxBOTTOM|
                           wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );
    sizer->Add( sizer2, 0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5 );
#endif

    sizer2 = new wxBoxSizer( wxVERTICAL);
    spin = new wxSpinCtrl( panel, ID_SETTINGS_SC_VIDEO_CROP_TOP, wxU(""),
                           wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 0, 2048, 2 );
    spin->Enable( FALSE );
    sizer2->Add( spin, 0, wxALL|wxALIGN_CENTER, 2 );
    sizer3 = new wxBoxSizer( wxHORIZONTAL);
    spin = new wxSpinCtrl( panel, ID_SETTINGS_SC_VIDEO_CROP_LEFT, wxU(""),
                           wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 0, 2048, 2 );
    spin->Enable( FALSE );
    sizer3->Add( spin, 0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 2 );
    spin = new wxSpinCtrl( panel, ID_SETTINGS_SC_VIDEO_CROP_RIGHT, wxU(""),
                           wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 0, 2048, 2 );
    spin->Enable( FALSE );
    sizer3->Add( spin, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 2 );
    sizer2->Add( sizer3, 0, wxALL|wxALIGN_CENTER );
    spin = new wxSpinCtrl( panel, ID_SETTINGS_SC_VIDEO_CROP_BOTTOM, wxU(""),
                           wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 0, 2048, 2 );
    spin->Enable( FALSE );
    sizer2->Add( spin, 0, wxALL|wxALIGN_CENTER, 2 );
    sizer->Add( sizer2, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );

    /* Size */
    sizer->Add( new wxStaticText( panel, -1, wxU("Size") ),
                0, wxALL|wxALIGN_LEFT, 5 );
    sizer2 = new wxBoxSizer( wxHORIZONTAL );
    spin = new wxSpinCtrl( panel, ID_SETTINGS_SC_VIDEO_WIDTH, wxU(""),
                           wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 0, 2048, 2 );
    spin->Enable( FALSE );
    sizer2->Add( spin, 0, wxALL, 5 );
    sizer2->Add( new wxStaticText( panel, -1, wxU("x") ), 0, wxALL, 5 );
    spin = new wxSpinCtrl( panel, ID_SETTINGS_SC_VIDEO_HEIGHT, wxU(""),
                           wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 0, 2048, 2 );
    spin->Enable( FALSE );
    sizer2->Add( spin, 0, wxALL, 5 );

    sizer->Add( sizer2, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );

#if 0
    sizer->Add( new wxStaticText( panel, -1, wxU("Autocrop: []\n"
                                                 "Crop: [] [] [] []\n"
                                                 "2 pass: []\n"
                                                 "Width: []\n"
                                                 "Height: []\n"
                                                 "Preview [..]" ) ),
                0, wxALL, 5 );
#endif
    panel->SetSizerAndFit( sizer );
    panel->SetAutoLayout( TRUE );
    panel->Layout();

    return panel;
}

static wxPanel *PanelSettingsAudio( wxWindow *p_parent )
{
    wxPanel *panel = new wxPanel( p_parent, -1,
                                  wxDefaultPosition, wxSize(200, 200));
    wxFlexGridSizer * sizer = new wxFlexGridSizer( 2, 1, 20 );
    wxArrayString aS;
    wxComboBox *combo;
    unsigned int i;

#if 0
    /* TODO in the right way (there is a few problem with file type) */
    /* Codec */
    sizer->Add( new wxStaticText( panel, -1, wxU("Codec") ),
                0, wxALL|wxALIGN_LEFT, 5 );
    aS.Clear();
    aS.Add( "Faac" );
    aS.Add( "Lame" );
    aS.Add( "Vorbis" );
    aS.Add( "Ac3" );
    aS.Add( "Mpga" );
    aS.Add( "LPCM" );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_VIDEO_CODEC, aS[0],
                            wxDefaultPosition, wxDefaultSize,
                            aS, wxCB_READONLY );
    sizer->Add( combo, 1, wxEXPAND|wxALL|wxALIGN_RIGHT, 5 );
#endif
    sizer->Add( new wxStaticText( panel, -1, wxU("Samplerate") ),
                1, wxEXPAND|wxALL|wxALIGN_LEFT, 5 );
    aS.Clear();
    for( i = 0; i < hb_audio_rates_count; i++ )
        aS.Add( wxString::Format( wxU("%d"), hb_audio_rates[i].rate ) );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_AUDIO_SAMPLERATE, wxU(""),
                            wxDefaultPosition, wxDefaultSize,
                            aS, wxCB_READONLY );
    sizer->Add( combo, 0, wxALL|wxALIGN_RIGHT, 5 );


    panel->SetSizerAndFit( sizer );

    return panel;
}

static wxPanel *PanelSettingsOthers( wxWindow *p_parent )
{
    wxPanel *panel = new wxPanel( p_parent, -1,
                                  wxDefaultPosition, wxSize(200, 200));

    wxBoxSizer *sizer_row = new wxBoxSizer( wxVERTICAL );
    wxFlexGridSizer *sizer = new wxFlexGridSizer( 2, 2, 20 );
    wxComboBox *combo;
    wxArrayString aS;
    int i;
    int i_count;

    
    /* CPU */
    sizer->Add( new wxStaticText( panel, -1, wxU( "CPU" ) ),
                1, wxEXPAND|wxALL|wxALIGN_LEFT );

    i_count = g_hbApp->GetSystemCpuCount();
    aS.Clear();
    for( i = 0; i < i_count; i++ )
        aS.Add( wxString::Format( wxU("%d"), i+1 ) );

    combo = new wxComboBox( panel, ID_SETTINGS_CB_CPU,
                            aS[g_hbApp->GetDefaultCpuCount()-1],
                            wxDefaultPosition, wxDefaultSize,
                            aS, wxCB_READONLY );
    sizer->Add( combo, 0, wxALL|wxALIGN_RIGHT );

    /* Priority */
    sizer->Add( new wxStaticText( panel, -1, wxU( "Priority" ) ),
                1, wxEXPAND|wxALL|wxALIGN_LEFT );

    aS.Clear();
    aS.Add( wxU("Highest") );
    aS.Add( wxU("Above Normal") );
    aS.Add( wxU("Normal") );
    aS.Add( wxU("Below Normal") );
    aS.Add( wxU("Lowest") );
    combo = new wxComboBox( panel, ID_SETTINGS_CB_PRIORITY,
                            aS[4-g_hbApp->GetDefaultPriority()],
                            wxDefaultPosition, wxDefaultSize,
                            aS, wxCB_READONLY );
    sizer->Add( combo, 0, wxALL|wxALIGN_RIGHT );
    combo->Enable( FALSE ); /* Marche pas pr le moment */

    sizer_row->Add( sizer, 0, wxEXPAND|wxALL, 5 );

    panel->SetSizerAndFit( sizer_row );

    return panel;
}


class hbWizardPageSettings: public wxWizardPageSimple
{
public:
    hbWizardPageSettings( wxWizard *p_wizard );
    virtual ~hbWizardPageSettings();

    void OnBrowse( wxCommandEvent &event );
    void OnTitle( wxCommandEvent &event );
    void OnChapter( wxCommandEvent &event );
    void OnRadio( wxCommandEvent &event );
    void OnAudio( wxCommandEvent &event );

    void SetTitle( int i_title = -1 );
    void OnPageChanging( wxWizardEvent &event );
    void OnPageChanged( wxWizardEvent &event );

private:
    DECLARE_EVENT_TABLE()
    wxFileDialog *dlgFile;
};

hbWizardPageSettings::hbWizardPageSettings( wxWizard *p_wizard ) :
                                                wxWizardPageSimple(p_wizard)
{
    wxNotebook *nb = new wxNotebook( this, ID_SETTINGS_NB );
    wxNotebookSizer *sizerN = new wxNotebookSizer( nb );

    /* General settings */
    nb->AddPage( PanelSettingsGeneral(nb), wxU("General"), TRUE );

    /* Advanced */
    nb->AddPage( PanelSettingsVideo(nb), wxU("Advanced Video"), FALSE );

    /* Advanced */
    nb->AddPage( PanelSettingsAudio(nb), wxU("Advanced Audio"), FALSE );
    
    /* Others */
    nb->AddPage( PanelSettingsOthers(nb), wxU("Others"), FALSE );


    hbWizardPageHeaders( this, "Settings", "Set your settings ;).",
                         sizerN );

    /* */
    dlgFile = NULL;
}

hbWizardPageSettings::~hbWizardPageSettings()
{
}

void hbWizardPageSettings::SetTitle( int i_title )
{
    wxArrayString &aT = *g_hbApp->GetTitles();
    wxArrayString *p_array;
    wxComboBox *title;
    wxComboBox *chp;
    wxComboBox *lg;
    wxComboBox *fps;
    wxComboBox *arate;
    wxString sL;
    bool bSetDefault = false;
    int i;

    /* Update title */
    title = (wxComboBox*)FindWindow( ID_SETTINGS_CB_TITLE );
    if( i_title < 0 )
    {
        title->Clear();
        title->Append( aT );

        i_title = g_hbApp->GetDefaultTitle();
        title->SetValue( aT[i_title] );
        bSetDefault = true;
    }

    g_hbApp->SetTitle( i_title );


    /* Update chapter start/end */
    chp = (wxComboBox*)FindWindow( ID_SETTINGS_CB_CHAPTER_START );
    chp->Clear();
    for( i = 0; i < g_hbApp->GetChaptersCount(); i++ )
        chp->Append( wxString::Format( wxU("%d"), i+1 ) );
    chp->SetValue( wxU("1") );

    chp = (wxComboBox*)FindWindow( ID_SETTINGS_CB_CHAPTER_END );
    chp->Clear();
    for( i = 0; i < g_hbApp->GetChaptersCount(); i++ )
        chp->Append( wxString::Format( wxU("%d"), i+1 ) );
    chp->SetValue( wxString::Format( wxU("%d"), g_hbApp->GetChaptersCount() ) );

    /* Update Audio language */
    lg = (wxComboBox*)FindWindow( ID_SETTINGS_CB_AUDIO_LANGUAGE );
    sL = lg->GetValue();
    lg->Clear();
    p_array = g_hbApp->GetTracksAudio();
    lg->Append( *p_array );

    if( bSetDefault || p_array->Index(sL) == wxNOT_FOUND )
    {
        sL = g_hbApp->GetDefaultAudio();
        if( p_array->Index( sL ) == wxNOT_FOUND )
            sL = (*p_array)[0];
    }
    lg->SetValue( sL );
    g_hbApp->SetTrackAudio( p_array->Index(sL) );/* Needed for audio settings */

    lg = (wxComboBox*)FindWindow( ID_SETTINGS_CB_SUBTITLE_LANGUAGE );
    sL = lg->GetValue();
    lg->Clear();
    p_array = g_hbApp->GetTracksSubtitle();
    lg->Append( *p_array );

    if( bSetDefault || p_array->Index(sL) == wxNOT_FOUND )
    {
        sL = g_hbApp->GetDefaultSubtitle();
        if( p_array->Index( sL ) == wxNOT_FOUND )
            sL = p_array->Last();
    }
    lg->SetValue( sL );

    if( bSetDefault || g_hbApp->GetDefaultVideoRateBase() != 0 )
    {
        int i_base = g_hbApp->GetDefaultVideoRateBase();

        if( i_base == 0 || !bSetDefault )
            i_base = g_hbApp->GetVideoRateBase();

        fps = (wxComboBox*)FindWindow( ID_SETTINGS_CB_VIDEO_FPS );
        for( i = 0; i  < hb_video_rates_count; i++ )
        {
            if( hb_video_rates[i].rate  == i_base )
            {
                fps->SetSelection( i );
                break;
            }
        }
    }

    if( bSetDefault || g_hbApp->GetDefaultAudioSamplerate() != 0 )
    {
        int i_rate = g_hbApp->GetDefaultAudioSamplerate();

        if( i_rate == 0 || !bSetDefault )
            i_rate = g_hbApp->GetAudioSamplerate();
        if( i_rate == 0 )
            i_rate = 44100;

        arate = (wxComboBox*)FindWindow( ID_SETTINGS_CB_AUDIO_SAMPLERATE );
        arate->SetValue( wxString::Format( wxU("%d"), i_rate ) );
    }

    /* Set auto crop values and width size */
    if( bSetDefault )
    {
        wxSpinCtrl *spin;
        int crop[4];
        int i_width, i_height;

        g_hbApp->GetVideoAutocrop( crop );
        g_hbApp->GetVideoSize( &i_width, &i_height );

        /* Crop */
        spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_CROP_TOP );
        spin->SetValue( crop[0] );
        spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_CROP_BOTTOM );
        spin->SetValue( crop[1] );
        spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_CROP_LEFT );
        spin->SetValue( crop[2] );
        spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_CROP_RIGHT );
        spin->SetValue( crop[3] );

        /* Size */
        spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_WIDTH );
        spin->SetValue( i_width - (crop[2]+crop[3]) );
        spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_HEIGHT );
        spin->SetValue( i_height - (crop[0]+crop[1]) );
    }
}


void hbWizardPageSettings::OnTitle( wxCommandEvent &event )
{
    SetTitle( event.GetSelection() );
}

void hbWizardPageSettings::OnChapter( wxCommandEvent &event )
{
    wxComboBox *start = (wxComboBox*)FindWindow( ID_SETTINGS_CB_CHAPTER_START );
    wxComboBox *end   = (wxComboBox*)FindWindow( ID_SETTINGS_CB_CHAPTER_END );

    if( event.GetId() == ID_SETTINGS_CB_CHAPTER_START )
    {
        if( start->GetSelection() > end->GetSelection() )
            end->SetSelection( start->GetSelection() );
    }
    else
    {
        if( end->GetSelection() < start->GetSelection() )
            start->SetSelection( end->GetSelection() );
    }
}

void hbWizardPageSettings::OnBrowse( wxCommandEvent &event )
{
    if( dlgFile == NULL )
        dlgFile = new wxFileDialog( this, wxU( "Choose a file"),
                                      wxGetCwd() );

    if( dlgFile->ShowModal() == wxID_OK )
    {
        const wxChar *p = dlgFile->GetPath();
        wxComboBox *file = (wxComboBox*)FindWindow(ID_SETTINGS_CB_OUTPUT_FILE);
        file->SetValue( p );
    }
}

void hbWizardPageSettings::OnRadio( wxCommandEvent &event )
{
    wxRadioButton *rSize =
        (wxRadioButton*)FindWindow( ID_SETTINGS_RD_OUTPUT_SIZE );
    wxRadioButton *rBitrate =
        (wxRadioButton*)FindWindow( ID_SETTINGS_RD_OUTPUT_BITRATE );
    wxComboBox *cSize =
        (wxComboBox*)FindWindow( ID_SETTINGS_CB_OUTPUT_SIZE );
    wxComboBox *cBitrate =
        (wxComboBox*)FindWindow( ID_SETTINGS_CB_OUTPUT_BITRATE );

    if( event.GetId() == ID_SETTINGS_RD_OUTPUT_SIZE )
    {
        /* FIXME is that needed ?? */
        rBitrate->SetValue(FALSE);
        cBitrate->Enable(FALSE);
        cSize->Enable(TRUE);
    }
    else
    {
        rSize->SetValue(FALSE);
        cSize->Enable(FALSE);
        cBitrate->Enable(TRUE);
    }
}
void hbWizardPageSettings::OnAudio( wxCommandEvent &event )
{
    if( g_hbApp->GetDefaultAudioSamplerate() == 0 )
    {
        int i_rate = g_hbApp->GetAudioSamplerate();
        wxComboBox *arate =
            (wxComboBox*)FindWindow( ID_SETTINGS_CB_AUDIO_SAMPLERATE );

        if( i_rate == 0 ) i_rate = 44100;
        arate->SetValue( wxString::Format( wxU("%d"), i_rate ) );
    }
}

void hbWizardPageSettings::OnPageChanging( wxWizardEvent &event )
{
    wxComboBox *cFile;
    wxComboBox *cChap;
    wxComboBox *cLang;
    wxComboBox *cQ;
    wxComboBox *cCombo;
    wxCheckBox *cCheck;
    wxRadioButton *rSize;
    wxSpinCtrl *spin;
    int i_bitrate_audio;
    int crop[4];
    int i_width, i_height;

    wxString sFile;

    if( !event.GetDirection() )
        return;

    /* We need to validate params and start encoding */

    /* Get and Check validity of parameters */
    cFile = (wxComboBox*)FindWindow( ID_SETTINGS_CB_OUTPUT_FILE );
    sFile = cFile->GetValue();
    if( sFile.IsEmpty() )
    {
        hbError( this, wxU("You have to give a output file name") );
        event.Veto();
        return;
    }
    g_hbApp->SetOutputFile( sFile );

    /* Chapter start/end */
    cChap = (wxComboBox*)FindWindow( ID_SETTINGS_CB_CHAPTER_START );
    g_hbApp->SetChapterStart( cChap->GetSelection() + 1 );

    cChap = (wxComboBox*)FindWindow( ID_SETTINGS_CB_CHAPTER_END );
    g_hbApp->SetChapterEnd( cChap->GetSelection() + 1 );

    /* Audio */
    cLang = (wxComboBox*)FindWindow( ID_SETTINGS_CB_AUDIO_LANGUAGE );
    g_hbApp->SetTrackAudio( cLang->GetSelection(), 0 );
    /* FIXME find a better way */
    cQ = (wxComboBox*)FindWindow( ID_SETTINGS_CB_AUDIO_QUALITY );
    sscanf( cQ->GetValue().ToAscii(), "%*s - %d kb/s", &i_bitrate_audio );
    g_hbApp->SetAudioBitrate( i_bitrate_audio );

    long arate;
    cCombo = (wxComboBox*)FindWindow( ID_SETTINGS_CB_AUDIO_SAMPLERATE );
    cCombo->GetValue().ToLong( &arate );
    g_hbApp->SetAudioSamplerate( arate );

    /* Subs */
    cLang = (wxComboBox*)FindWindow( ID_SETTINGS_CB_SUBTITLE_LANGUAGE );
    g_hbApp->SetTrackSubtitle( cLang->GetSelection() );

    /* Video/Quality */
    cQ = (wxComboBox*)FindWindow( ID_SETTINGS_CB_VIDEO_QUALITY );
    if( cQ->GetSelection() == 3 )
        g_hbApp->SetVideoPass( 2 );
    else
        g_hbApp->SetVideoPass( 1 );
    /* Video codec */
    cCombo = (wxComboBox*)FindWindow( ID_SETTINGS_CB_VIDEO_CODEC );
    g_hbApp->SetVideoCodec( cCombo->GetValue() );
    /* Video fps */
    cCombo = (wxComboBox*)FindWindow( ID_SETTINGS_CB_VIDEO_FPS );
    g_hbApp->SetVideoRateBase( hb_video_rates[cCombo->GetSelection()].rate );
    /* Deinterlace */
    cCheck = (wxCheckBox*)FindWindow( ID_SETTINGS_CH_VIDEO_DEINTERLACE );
    g_hbApp->SetVideoDeinterlace( cCheck->GetValue() );
    /* Crop */
    cCheck = (wxCheckBox*)FindWindow( ID_SETTINGS_CH_VIDEO_CROP );
    if( cCheck->GetValue() )
    {
        spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_CROP_TOP );
        crop[0] = spin->GetValue();
        spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_CROP_BOTTOM );
        crop[1] = spin->GetValue();
        spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_CROP_LEFT );
        crop[2] = spin->GetValue();
        spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_CROP_RIGHT );
        crop[3] = spin->GetValue();
        g_hbApp->SetVideoCrop( crop );
    }
    else
    {
        g_hbApp->SetVideoCrop( NULL );
    }
    spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_WIDTH );
    i_width = spin->GetValue();
    spin = (wxSpinCtrl*)FindWindow( ID_SETTINGS_SC_VIDEO_HEIGHT );
    i_height = spin->GetValue();
    g_hbApp->SetVideoSize( i_width, i_height );


    rSize = (wxRadioButton*)FindWindow( ID_SETTINGS_RD_OUTPUT_SIZE );
    if( rSize->GetValue() )
    {
        wxComboBox *cSize =
            (wxComboBox*)FindWindow( ID_SETTINGS_CB_OUTPUT_SIZE );
        long i_size;
        
        cSize->GetValue().ToLong( &i_size );
        if( i_size <= 0 )
        {
            hbError( this, wxU("Invalide file size") );
            event.Veto();
            return;
        }
        g_hbApp->SetTotalSize( i_size );
    }
    else
    {
        wxComboBox *cBitrate =
            (wxComboBox*)FindWindow( ID_SETTINGS_CB_OUTPUT_BITRATE );
        long i_bitrate;

	cBitrate->GetValue().ToLong( &i_bitrate );
        if( i_bitrate<= 0 )
        {
            hbError( this, wxU("Invalid total bitrate") );
            event.Veto();
            return;
        }
        else if( i_bitrate <= i_bitrate_audio )
        {
            hbError( this, wxU("Incompatible total bitrate with audio bitrate") );
            event.Veto();
            return;
        }
        g_hbApp->SetTotalBitrate( i_bitrate );
    }

    /* Other settings */
    /* CPU */
    cCombo = (wxComboBox*)FindWindow( ID_SETTINGS_CB_CPU );
    g_hbApp->SetCpuCount( cCombo->GetSelection() );

    cCombo = (wxComboBox*)FindWindow( ID_SETTINGS_CB_PRIORITY );
    g_hbApp->SetPriority( cCombo->GetSelection() );

    /* TODO finish */

    /* Start the encode (TODO) */
    if( g_hbApp->Encode() < 0 )
    {
        event.Veto();   /* Wrong */
        return;
    }
}
void hbWizardPageSettings::OnPageChanged( wxWizardEvent &event )
{
    /* Initialise avec le titre par défaut */
    SetTitle();
}

BEGIN_EVENT_TABLE( hbWizardPageSettings, wxWizardPageSimple )
    EVT_BUTTON( ID_SETTINGS_BT_FILE, hbWizardPageSettings::OnBrowse )
    EVT_COMBOBOX( ID_SETTINGS_CB_TITLE, hbWizardPageSettings::OnTitle )
    EVT_COMBOBOX( ID_SETTINGS_CB_CHAPTER_START,hbWizardPageSettings::OnChapter)
    EVT_COMBOBOX( ID_SETTINGS_CB_CHAPTER_END, hbWizardPageSettings::OnChapter )
    EVT_COMBOBOX( ID_SETTINGS_CB_AUDIO_LANGUAGE, hbWizardPageSettings::OnAudio )
    EVT_WIZARD_PAGE_CHANGING( -1, hbWizardPageSettings::OnPageChanging )
    EVT_WIZARD_PAGE_CHANGED( -1, hbWizardPageSettings::OnPageChanged )
    EVT_RADIOBUTTON( ID_SETTINGS_RD_OUTPUT_SIZE, hbWizardPageSettings::OnRadio)
    EVT_RADIOBUTTON( ID_SETTINGS_RD_OUTPUT_BITRATE, hbWizardPageSettings::OnRadio)
END_EVENT_TABLE()


/*****************************************************************************
 * hbWizardPageEncode:
 *****************************************************************************/
class hbWizardPageEncode: public wxWizardPageSimple
{
public:
    hbWizardPageEncode( wxWizard *p_wizard );
    virtual ~hbWizardPageEncode();

private:
    //DECLARE_EVENT_TABLE()
};

hbWizardPageEncode::hbWizardPageEncode( wxWizard *p_wizard ) :
                                             wxWizardPageSimple(p_wizard)
{
    wxBoxSizer *sizer;
    sizer = new wxBoxSizer( wxVERTICAL );
#if 0
    wxGauge * gauge;
    sizer->Add( new wxStaticText( this, -1, wxU("Progess: x%%" ) ) );

    gauge = new wxGauge( this, -1, 100 );
    gauge->SetValue( 50 );

    sizer->Add( gauge );
#endif


    hbWizardPageHeaders( this, "Success",
                         "The encode is finished",
                         sizer );

}

hbWizardPageEncode::~hbWizardPageEncode()
{
}

/****************************************************************************
 * hbWizard:
 ****************************************************************************/
hbWizard::hbWizard( wxWindow *p_parent) :
     wxWizard( p_parent, -1, wxU("HandBrake"), wxNullBitmap, wxDefaultPosition)
{
    page1 = new hbWizardPageSource( this );
    page2 = new hbWizardPageSettings( this );
    page3 = new hbWizardPageEncode( this );

    wxWizardPageSimple::Chain( page1, page2 );
    wxWizardPageSimple::Chain( page2, page3 );

    //SetPageSize( wxSize( 500, 400 ) );
    FitToPage( page1 );
    FitToPage( page2 );
    FitToPage( page3 );
}

hbWizard::~hbWizard()
{
    Destroy();
    /* Is that ok */
    delete page1;
    delete page2;
    delete page3;
}

void hbWizard::Run()
{
    fprintf( stderr, "hbWizard::Run\n" );

    RunWizard( page1 );
}


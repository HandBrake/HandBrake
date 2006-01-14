/*****************************************************************************
 * wizard.h:
 *****************************************************************************
 * Copyright (C) 
 * $Id: hbWizard.h,v 1.1 2005/01/16 15:59:21 titer Exp $
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

class hbWizardPageSource;
class hbWizardPageSettings;
class hbWizardPageEncode;

class hbWizard: public wxWizard
{
public:
    hbWizard( wxWindow *p_parent );
    virtual ~hbWizard();

    void Run();

private:
    int i_essai;
    hbWizardPageSource   *page1;
    hbWizardPageSettings *page2;
    hbWizardPageEncode   *page3;

    //DECLARE_EVENT_TABLE()
};

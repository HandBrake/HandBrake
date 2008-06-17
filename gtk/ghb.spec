Name:		handbrake
Version:	0.9.2
Release:	1%{?dist}
Summary:	A program to rip and encode DVDs and other sources to MPEG-4

Group:		Applications/Multimedia
License:	GPL
URL:		http://handbrake.fr/
Source0:	HandBrake.tgz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires:	glib2 >= 2.16, gtk2 >= 2.12, hal-libs

%description
HandBrake is an open-source, GPL-licensed, multiplatform, multithreaded 
DVD to MPEG-4 converter, available for MacOS X, Linux and Windows.

%prep
%setup -q


%build
%configure
jam


%install
rm -rf $RPM_BUILD_ROOT
DESTDIR=$RPM_BUILD_ROOT jam install 

## blow away stuff we don't want
/bin/rm $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/icon-theme.cache

%clean
rm -rf $RPM_BUILD_ROOT

%post
touch --no-create %{_datadir}/icons/hicolor
if [ -x /usr/bin/gtk-update-icon-cache ]; then
  gtk-update-icon-cache -q %{_datadir}/icons/hicolor
fi

%postun
/sbin/ldconfig
touch --no-create %{_datadir}/icons/hicolor
if [ -x /usr/bin/gtk-update-icon-cache ]; then
  gtk-update-icon-cache -q %{_datadir}/icons/hicolor
fi

%files
%defattr(-,root,root,-)
%doc %{_datadir}/doc
%{_datadir}/ghb
%{_datadir}/icons
%{_datadir}/locale
%{_datadir}/applications
%{_bindir}


%changelog
* Sat May 31 2008 John Stebbins <jstebbins@jetheaddev.com> 
- Initial release



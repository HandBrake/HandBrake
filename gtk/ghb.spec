%define name HandBrake
%define release 1

Name:		%{name}
Version:	%{version}
Release:	%{release}%{?dist}
Summary:	A program to transcode DVDs and other sources to MPEG-4

Group:		Applications/Multimedia
License:	GPL
URL:		http://handbrake.fr/
Vendor:		The HandBrake Project
Source0:	%{name}-%{version}.tar.bz2
Prefix:		%{_prefix}
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires:	glib2 >= 2.16, gtk2 >= 2.12, hal-libs, webkitgtk, gstreamer
Requires:	gstreamer-plugins-base

%description
HandBrake is an open-source, GPL-licensed, multiplatform, multithreaded 
transcoder, available for MacOS X, Linux and Windows.

%package gui
Summary:	A program to transcode DVDs and other sources to MPEG-4
Group:		Applications/Multimedia

%package cli
Summary:	A program to transcode DVDs and other sources to MPEG-4
Group:		Applications/Multimedia

%description gui
HandBrake is an open-source, GPL-licensed, multiplatform, multithreaded 
transcoder, available for MacOS X, Linux and Windows.

%description cli
HandBrake is an open-source, GPL-licensed, multiplatform, multithreaded 
transcoder, available for MacOS X, Linux and Windows.

%prep
%setup -n %{name}-%{version} -D -T
#%setup -q
#cd %{_builddir}/%{name}-%{version}


%build
#./configure --prefix=%{_prefix}
#make -C build


%install
#rm -rf $RPM_BUILD_ROOT
# I don't want to rebuild the world, so just install what I've prebuilt
make -C $RPM_BUILD_ROOT/../.. DESTDIR=$RPM_BUILD_ROOT install

## blow away stuff we don't want
/bin/rm -f $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/icon-theme.cache

%clean
rm -rf $RPM_BUILD_ROOT

%post gui
touch --no-create %{_datadir}/icons/hicolor
if [ -x /usr/bin/gtk-update-icon-cache ]; then
  gtk-update-icon-cache -q %{_datadir}/icons/hicolor
fi

%postun gui
touch --no-create %{_datadir}/icons/hicolor
if [ -x /usr/bin/gtk-update-icon-cache ]; then
  gtk-update-icon-cache -q %{_datadir}/icons/hicolor
fi

%files gui
%defattr(-,root,root,-)
%doc NEWS AUTHORS CREDITS THANKS COPYING
%{_datadir}/icons
%{_datadir}/applications
%{_bindir}/ghb

%files cli
%defattr(-,root,root,-)
%doc NEWS AUTHORS CREDITS THANKS COPYING
%{_bindir}/HandBrakeCLI

%changelog
* Sat May 31 2008 John Stebbins <jstebbins@jetheaddev.com> 
- Initial release



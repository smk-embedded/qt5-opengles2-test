Name:		qt5-opengles2-test
Version:	1.0.2
Release:	1
Summary:	OpenGL ES 2.0 rendering + multi-touch input test application

Group:		System/GUI/Other
License:	MIT
URL:		https://github.com/thp/qt5-opengles2-test
Source0:	%{name}-%{version}.tar.bz2
BuildRequires:	pkgconfig(Qt5Gui)

%description
This application is used to test OpenGL ES 2.0 rendering in
a simple QWindow, plus multi-touch input, window orientation,
window focus handling and some other game-related features.

%prep
%setup -q -n %{name}-%{version}


%build
%qmake5
make %{?_smp_mflags}


%install
make INSTALL_ROOT=%{buildroot} install


%files
%defattr(-,root,root,-)
/usr/bin/%{name}
/usr/share/applications/%{name}.desktop


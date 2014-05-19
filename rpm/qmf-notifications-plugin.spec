Name:       qmf-notifications-plugin
Summary:    Notifications plugin for Qt Messaging Framework (QMF)
Version:    0.0.18
Release:    1
Group:      System/Libraries
License:    BSD
URL:        https://github.com/nemomobile/qmf-notifications-plugin
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(qmfclient5)
BuildRequires:  pkgconfig(qmfmessageserver5)
BuildRequires:  pkgconfig(accounts-qt5)
BuildRequires:  pkgconfig(nemotransferengine-qt5)
BuildRequires:  pkgconfig(nemonotifications-qt5)
BuildRequires:  qt5-qttools-linguist

%description
Notifications plugin for Qt Messaging Framework (QMF)

%prep
%setup -q -n %{name}-%{version}

%build

%qmake5 QMF_INSTALL_ROOT=/usr

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%qmake5_install

%files
%defattr(-,root,root,-)
%{_libdir}/qmf/plugins5/messageserverplugins/libnotifications.so
%{_datadir}/lipstick/notificationcategories/x-nemo.email.conf
%{_datadir}/lipstick/notificationcategories/x-nemo.email.error.conf

%{_datadir}/translations/qmf-notifications_eng_en.qm

%package ts-devel
Summary:    Translation source for qmf-notifications-plugin
License:    BSD
Group:      System/Applications

%description ts-devel
Translation source for qmf-notifications-plugin

%files ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/qmf-notifications.ts

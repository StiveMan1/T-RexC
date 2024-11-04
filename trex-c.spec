Name:           trex-c
Version:        1.0
Release:        1%{?dist}
Summary:        A console game based on the Google T-Rex game
License:        GNU
URL:            https://github.com/StiveMan1/T-RexC
Source0:        trex-c.c
Source1:        game_objects.h

BuildRequires:  gcc
Requires:       glibc

%description
This is a console game inspired by the Google T-Rex game, where players can enjoy an adventure in a command-line interface.

%prep
# Prepare the build environment
%setup -c

%build
gcc -o trex-c %{_builddir}/trex-c.c -lpthread -lm -O3

%install
mkdir -p %{buildroot}/usr/bin
install -Dm755 trex-c %{buildroot}/usr/bin/trex-c

%files
/usr/bin/trex-c

%changelog
* Sat Nov 04 2024 Sanzhar Zhanalin <04024004zh@gmail.com> - 1.0-1
- Initial release


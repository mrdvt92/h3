Name:           h3
Version:        3.7.1
Release:        4%{?dist}
Summary:        H3 Hexagonal Hierarchical Geospatial Indexing System Package

Group:          Development/Libraries
License:        Apache License 2.0
URL:            https://github.com/uber/h3
Source0:        h3-3.7.1.tar.gz

BuildRequires:  cmake3
BuildRequires:  make
BuildRequires:  gcc
BuildRequires:  doxygen
BuildRequires:  graphviz

%description
H3 is a Hexagonal Hierarchical Geospatial Indexing System.

%package devel
Summary:        H3 Hexagonal Hierarchical Geospatial Indexing System Development Header Package

%description devel
H3 is a Hexagonal Hierarchical Geospatial Indexing System.

%prep
%setup -q

%build
mkdir build
pushd build
#cmake3 -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_CXX_IMPLICIT_LINK_DIRECTORIES=/usr/lib64 -DINSTALL_LIB_DIR=/usr/lib64 -DENABLE_DOCS=YES -DBUILD_SHARED_LIBS=true ..
cmake3 -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_DOCS=YES -DBUILD_SHARED_LIBS=true -DCMAKE_INSTALL_LIBDIR=/usr/lib64 -DLIBRARY_OUTPUT_PATH=/usr/lib64 ..
make %{?_smp_mflags}
make docs
make kml
popd

%check
pushd build
#make test
popd

%install
pushd build
make install DESTDIR=%{buildroot} LIB_SUFFIX=64 LIB_INSTALL_DIR=/usr/lib64
mkdir -p %{buildroot}%{_datadir}/%{name}/html/
cp dev-docs/_build/html/* %{buildroot}%{_datadir}/%{name}/html/
mkdir -p %{buildroot}%{_datadir}/%{name}/kml/
cp KML/*.kml %{buildroot}%{_datadir}/%{name}/kml/
#TODO how to do this in cmake/make
rm -r %{buildroot}%{_prefix}/lib/cmake
#TODO how to do this in cmake/make
mv %{buildroot}/usr/lib %{buildroot}/usr/lib64
find %{buildroot} -type f
popd

%files
%defattr(0644,root,root,-)
%attr(0755,root,root) %{_bindir}/geoToH3
%attr(0755,root,root) %{_bindir}/h3ToComponents
%attr(0755,root,root) %{_bindir}/h3ToGeo
%attr(0755,root,root) %{_bindir}/h3ToGeoBoundary
%attr(0755,root,root) %{_bindir}/h3ToGeoBoundaryHier
%attr(0755,root,root) %{_bindir}/h3ToGeoHier
%attr(0755,root,root) %{_bindir}/h3ToHier
%attr(0755,root,root) %{_bindir}/h3ToLocalIj
%attr(0755,root,root) %{_bindir}/hexRange
%attr(0755,root,root) %{_bindir}/kRing
%attr(0755,root,root) %{_bindir}/localIjToH3
%{_libdir}/libh3.so.1
%{_libdir}/libh3.so
%doc %{_datadir}/%{name}/html/*
%doc %{_datadir}/%{name}/kml/*

%files devel
%defattr(0644,root,root,-)
%{_includedir}/h3/h3api.h

%changelog
* Tue Nov  3 2020 Michael R. Davis <mrdvt92@yahoo.com> - 3.1.7-2
- Original SPEC Package

* Tue Nov  3 2020 Michael R. Davis <mrdvt92@yahoo.com> - 3.1.7-3
- Added .so
- Added docs .html and .kml
- Dropped .a

* Tue Nov  3 2020 Michael R. Davis <mrdvt92@yahoo.com> - 3.1.7-4
- Moved .h to devel package

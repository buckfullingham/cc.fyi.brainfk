<%!
import conan
%>

[buildenv]
CC=gcc
CXX=g++

[settings]
os=${conan.internal.api.detect.detect_api.detect_os()}
arch=${conan.internal.api.detect.detect_api.detect_arch()}
compiler=gcc
compiler.version=13
compiler.cppstd=23
compiler.libcxx=libstdc++

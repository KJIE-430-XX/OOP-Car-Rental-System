# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/mnt/c/Car Rental System -- OOP/OOP-Car-Rental-System/build-linux/_deps/crow-src")
  file(MAKE_DIRECTORY "/mnt/c/Car Rental System -- OOP/OOP-Car-Rental-System/build-linux/_deps/crow-src")
endif()
file(MAKE_DIRECTORY
  "/mnt/c/Car Rental System -- OOP/OOP-Car-Rental-System/build-linux/_deps/crow-build"
  "/mnt/c/Car Rental System -- OOP/OOP-Car-Rental-System/build-linux/_deps/crow-subbuild/crow-populate-prefix"
  "/mnt/c/Car Rental System -- OOP/OOP-Car-Rental-System/build-linux/_deps/crow-subbuild/crow-populate-prefix/tmp"
  "/mnt/c/Car Rental System -- OOP/OOP-Car-Rental-System/build-linux/_deps/crow-subbuild/crow-populate-prefix/src/crow-populate-stamp"
  "/mnt/c/Car Rental System -- OOP/OOP-Car-Rental-System/build-linux/_deps/crow-subbuild/crow-populate-prefix/src"
  "/mnt/c/Car Rental System -- OOP/OOP-Car-Rental-System/build-linux/_deps/crow-subbuild/crow-populate-prefix/src/crow-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/mnt/c/Car Rental System -- OOP/OOP-Car-Rental-System/build-linux/_deps/crow-subbuild/crow-populate-prefix/src/crow-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/mnt/c/Car Rental System -- OOP/OOP-Car-Rental-System/build-linux/_deps/crow-subbuild/crow-populate-prefix/src/crow-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()

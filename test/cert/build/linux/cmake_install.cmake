# Install script for directory: /mnt/e/work/bubi/github/bumo/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/buchain/scripts" TYPE PROGRAM FILES
    "/mnt/e/work/bubi/github/bumo/src/../deploy/bumo"
    "/mnt/e/work/bubi/github/bumo/src/../deploy/bumod"
    "/mnt/e/work/bubi/github/bumo/src/../deploy/start-stop-daemon"
    )
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/buchain/bin" TYPE DIRECTORY FILES "/mnt/e/work/bubi/github/bumo/src/../bin/" FILES_MATCHING REGEX "/[^/]*\\.bin$" REGEX "/[^/]*\\.dat$")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/buchain/config" TYPE DIRECTORY FILES "/mnt/e/work/bubi/github/bumo/src/../build/win32/config/" FILES_MATCHING REGEX "/bumo\\.json$" REGEX "/bumo\\-mainnet\\.json$" REGEX "/bumo\\-testnet\\.json$" REGEX "/bumo\\-single\\.json$")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/buchain/jslib" TYPE DIRECTORY FILES "/mnt/e/work/bubi/github/bumo/src/../build/win32/jslib/" FILES_MATCHING REGEX "/[^/]*\\.js$")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/mnt/e/work/bubi/github/bumo/build/linux/3rd/http/cmake_install.cmake")
  include("/mnt/e/work/bubi/github/bumo/build/linux/utils/cmake_install.cmake")
  include("/mnt/e/work/bubi/github/bumo/build/linux/common/cmake_install.cmake")
  include("/mnt/e/work/bubi/github/bumo/build/linux/cert_manager/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/mnt/e/work/bubi/github/bumo/build/linux/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")

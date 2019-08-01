cmake_minimum_required(VERSION 2.8)

project(cert)
add_compile_options(-g -O2)
add_definitions(-DSVNVERSION=\"${SVNVERSION}\")
message(STATUS "-DSVNVERSION="${SVNVERSION})

set( V8_LIB_PATH "linux")
set( OS_NAME "OS_LINUX")
IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
    MESSAGE(STATUS "current platform: Linux ")  
	set( OS_NAME "OS_LINUX")
	set(CERT_LINKER_FLAGS "-Wl,-dy -lpthread -lrt -ldl")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")  
    MESSAGE(STATUS "current platform: Darwin")  
	set( OS_NAME "OS_MAC")
	set(CERT_LINKER_FLAGS "-Wl, -lpthread -ldl")
	set( V8_LIB_PATH "mac")
ELSE ()  
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")  
ENDIF ()  

set(BUMOCHAIN buchain)
set(CERT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CERT_ROOT_DIR ${CERT_SRC_DIR}/..)
set(LIBRARY_OUTPUT_PATH ${CERT_ROOT_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CERT_ROOT_DIR}/bin)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

execute_process(COMMAND make all WORKING_DIRECTORY ${CERT_SRC_DIR}/3rd)

include_directories(
    ${CERT_SRC_DIR}
    ${CERT_SRC_DIR}/3rd/jsoncpp/include
    ${CERT_SRC_DIR}/3rd/rocksdb/include
    ${CERT_SRC_DIR}/3rd/bzip2-1.0.6
    ${CERT_SRC_DIR}/3rd/zlib-1.2.8
    ${CERT_SRC_DIR}/3rd/asio/include
    ${CERT_SRC_DIR}/3rd/openssl/include
    ${CERT_SRC_DIR}/3rd/http
)

link_directories(
    ${CERT_SRC_DIR}/3rd/rocksdb
    ${CERT_SRC_DIR}/3rd/jsoncpp/libs
    ${CERT_SRC_DIR}/3rd/openssl
    ${CERT_SRC_DIR}/3rd/zlib-1.2.8
    ${CERT_SRC_DIR}/3rd/bzip2-1.0.6
    ${CERT_SRC_DIR}/3rd/basic/lib
    ${CERT_SRC_DIR}/3rd/curl/lib/.libs/
    ${LIBRARY_OUTPUT_PATH}
)

set(CERT_DEPENDS_LIBS rocksdb json ssl crypto z bz2)

add_subdirectory(3rd/http)
add_subdirectory(utils)
add_subdirectory(common)
add_subdirectory(cert_manager)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
	set(CERT_SCRIPTS ${CERT_ROOT_DIR}/deploy)
	install(
		PROGRAMS ${CERT_SCRIPTS}/bumo ${CERT_SCRIPTS}/bumod ${CERT_SCRIPTS}/start-stop-daemon
		DESTINATION ${BUMOCHAIN}/scripts
	)
ENDIF ()  

install(
    DIRECTORY ${CERT_ROOT_DIR}/bin/
    DESTINATION ${BUMOCHAIN}/bin
    FILES_MATCHING PATTERN "*.bin" PATTERN "*.dat"
)

install(
    DIRECTORY ${CERT_ROOT_DIR}/build/win32/config/
    DESTINATION ${BUMOCHAIN}/config
    FILES_MATCHING PATTERN "bumo.json" PATTERN "bumo-mainnet.json" PATTERN "bumo-testnet.json" PATTERN "bumo-single.json"
)

install(
    DIRECTORY ${CERT_ROOT_DIR}/build/win32/jslib/
    DESTINATION ${BUMOCHAIN}/jslib
    FILES_MATCHING PATTERN "*.js"
)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")  
	set(SUPPLEMENT ${CERT_ROOT_DIR}/build/linux/MakeSupplement)
	set(FIX_DIR ${CMAKE_INSTALL_PREFIX}/${certCHAIN})
	message(STATUS "install dir: ${FIX_DIR}")

	file(WRITE ${SUPPLEMENT} "soft_link:\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s ${FIX_DIR}/scripts/cert /etc/init.d/cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s ${FIX_DIR}/scripts/certd /etc/init.d/certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/cert /etc/rc1.d/S99cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/cert /etc/rc1.d/S99cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/cert /etc/rc2.d/S99cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/cert /etc/rc3.d/S99cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/cert /etc/rc4.d/S99cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/cert /etc/rc5.d/S99cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/certd /etc/rc1.d/S50certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/certd /etc/rc1.d/S50certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/certd /etc/rc2.d/S50certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/certd /etc/rc3.d/S50certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/certd /etc/rc4.d/S50certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo ln -s -f /etc/init.d/certd /etc/rc5.d/S50certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/data;\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/coredump;\n")

	file(APPEND ${SUPPLEMENT} "\nuninstall:\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc1.d/S99cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc2.d/S99cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc3.d/S99cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc4.d/S99cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc5.d/S99cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/init.d/cert;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc1.d/S50certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc2.d/S50certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc3.d/S50certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc4.d/S50certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/rc5.d/S50certd;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -f /etc/init.d/bumod;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/bin;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/cfca;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/config;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/scripts\n")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")  
    set(SUPPLEMENT ${CERT_ROOT_DIR}/build/mac/MakeSupplement)
	set(FIX_DIR ${CMAKE_INSTALL_PREFIX}/${BUMOCHAIN})
	message(STATUS "install dir: ${FIX_DIR}")

	file(WRITE ${SUPPLEMENT} "soft_link:\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/data;\n")
	file(APPEND ${SUPPLEMENT} "\tsudo mkdir -p ${FIX_DIR}/coredump;\n")

	file(APPEND ${SUPPLEMENT} "\nuninstall:\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/bin;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/cfca;\\\n")
	file(APPEND ${SUPPLEMENT} "\trm -rf ${FIX_DIR}/config;\\\n")
ELSE ()  
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")  
ENDIF ()  

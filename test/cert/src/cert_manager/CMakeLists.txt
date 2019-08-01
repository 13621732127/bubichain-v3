#bumo main module CmakeLists.txt -- bumo

set(APP_CERT cert)

set(APP_CERT_SRC
    main.cpp
    cert_configure.cpp
    cert_server.cpp
	cert_storage.cpp
)

set(INNER_LIBS cert_common cert_utils cert_http)

#Generate executable files
add_executable(${APP_CERT} ${APP_CERT_SRC})

#Specify dependent libraries for target objects
	
IF (${OS_NAME} MATCHES "OS_LINUX")  
    MESSAGE(STATUS "current platform: Linux ")  
	target_link_libraries(${APP_CERT}
    -Wl,-dn ${INNER_LIBS})
ELSE ()  
	MESSAGE(STATUS "current platform: MAC ")  
	add_definitions(${CERT_LINKER_FLAGS})
	target_link_libraries(${APP_CERT} ${INNER_LIBS} ${CERT_DEPENDS_LIBS})
ENDIF () 


#Specify compiling options for target objets
target_compile_options(${APP_CERT}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
)

install(TARGETS ${APP_CERT}
    RUNTIME DESTINATION ${BUMOCHAIN}/bin
)

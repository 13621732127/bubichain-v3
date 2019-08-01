#bumo common module CmakeLists.txt -- libbumo_common.a

set(LIB_CERT_COMMON cert_common)
set(COMMON_SRC
    configure_base.cpp storage.cpp argument.cpp
)

#Generate static library files
add_library(${LIB_CERT_COMMON} ${COMMON_SRC}) 

#Specify compiling options for target objects
target_compile_options(${LIB_CERT_COMMON}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D_WEBSOCKETPP_CPP11_STL_
    PUBLIC -D${OS_NAME}
	PUBLIC -fPIC
)

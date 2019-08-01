#bumo http module CmakeLists.txt -- libbumo_http.a

set(LIB_CERT_HTTP cert_http)
aux_source_directory(${CERT_SRC_DIR}/3rd/http/ HTTP_SRC)

add_library(${LIB_CERT_HTTP} ${HTTP_SRC})

target_compile_options(${LIB_CERT_HTTP}
    PUBLIC -std=c++11 
    PUBLIC -DASIO_STANDALONE
    PUBLIC -D${OS_NAME}
)

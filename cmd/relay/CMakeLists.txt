
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

###
### Executable
###
set(APP_NAME "relay_app")
add_executable(${APP_NAME} ${HEADERS} ${SOURCES} main.cc)
target_link_libraries(${APP_NAME} quicr OpenSSL::Crypto gsl sframe)
target_include_directories( ${APP_NAME} PRIVATE ../../include )


###
### Library Config
###

set(LIB_NAME "relay")

add_library(${LIB_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${LIB_NAME} quicr OpenSSL::Crypto gsl sframe)
target_include_directories( ${APP_NAME} PRIVATE ../../include )
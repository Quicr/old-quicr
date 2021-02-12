set(APP_NAME "relay")

file(GLOB_RECURSE APP_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh")
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

add_executable(${APP_NAME} ${APP_HEADERS} ${APP_SOURCES} main.cc)
target_link_libraries(${APP_NAME} quicr OpenSSL::Crypto gsl sframe)
target_include_directories( ${APP_NAME} PRIVATE ../../include )

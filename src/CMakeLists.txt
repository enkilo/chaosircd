set(LEXER_SOURCE src/ircd.l)
set(PARSER_SOURCE src/ircd.y)

include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/ConfigSharedLib.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/UseMySQL.cmake")
include("${LIBCHAOS_SOURCE_DIR}/CMakeModules/UseFreeType.cmake")
include("${CMAKE_SOURCE_DIR}/CMakeModules/ConfigGNUVars.cmake")
include(FindBISON)

bison_target(ircd-parser ircd.y ${CMAKE_CURRENT_BINARY_DIR}/ircd-parser.c
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/y.tab.h)
flex_target(ircd-scanner ircd.l ${CMAKE_CURRENT_BINARY_DIR}/ircd-lexer.c)
add_flex_bison_dependency(ircd-scanner ircd-parser)

set(SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/ircd-parser.c
    ${CMAKE_CURRENT_BINARY_DIR}/ircd-lexer.c
    chanmode.c
    channel.c
    chanuser.c
    chars.c
    class.c
    client.c
    conf.c
    lclient.c
    msg.c
    numeric.c
    oper.c
    ircd.c
    server.c
    service.c
    user.c
    usermode.c
    ${MSINTTYPES_HEADERS}
    "${CMAKE_SOURCE_DIR}/include/ircd/chanmode.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/channel.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/chanuser.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/chars.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/class.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/client.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/conf.h"
    "${CMAKE_BINARY_DIR}/include/ircd/config.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/ircd.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/lclient.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/msg.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/numeric.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/oper.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/server.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/service.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/user.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/usermode.h")

add_definitions(-DHAVE_SSL=1)
if(WINDOWS OR CYGWIN)

  add_library(ircd SHARED ${SOURCES})
  set_target_properties(ircd PROPERTIES INSTALL_RPATH
                                        "${CMAKE_INSTALL_PREFIX}/lib")
  target_compile_definitions(ircd PUBLIC "-DBUILD_IRCD=1")

  add_executable(
    chaosircd
    main.c "${CMAKE_SOURCE_DIR}/include/ircd/ircd.h"
    "${CMAKE_BINARY_DIR}/include/ircd/config.h"
    "${CMAKE_SOURCE_DIR}/include/ircd/ircd.h")
  set_target_properties(chaosircd PROPERTIES INSTALL_RPATH
                                             "${CMAKE_INSTALL_PREFIX}/lib")

  link_directories("${CMAKE_CURRENT_BINARY_DIR}/../lib"
                   "${LIBCHAOS_BINARY_DIR}")

  target_link_libraries(ircd chaos ${SOCKET_LIBRARIES})
  target_link_libraries(chaosircd ircd)

else()
  link_directories("${CMAKE_CURRENT_BINARY_DIR}/../lib")
  add_executable(chaosircd ${SOURCES} main.c)

  link_directories("${LIBCHAOS_BINARY_DIR}")

  target_link_libraries(chaosircd chaos ${MYSQL_LIBS} ${OPENSSL_LIBS}
                        ${SOCKET_LIBRARIES})
endif()

install(TARGETS chaosircd RUNTIME DESTINATION bin)

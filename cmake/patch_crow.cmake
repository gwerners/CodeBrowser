# Patch Crow's http_server.h to add SO_REUSEPORT on Linux.
# This allows fast server restart without TIME_WAIT delays.
#
# Applied automatically by CMake FetchContent PATCH_COMMAND.

set(SERVER_H "${CROW_DIR}/include/crow/http_server.h")

if(NOT EXISTS "${SERVER_H}")
  message(WARNING "Crow http_server.h not found at ${SERVER_H}, skipping patch")
  return()
endif()

file(READ "${SERVER_H}" CONTENT)

# Only patch once (check if already patched)
string(FIND "${CONTENT}" "SO_REUSEPORT" ALREADY_PATCHED)
if(NOT ALREADY_PATCHED EQUAL -1)
  message(STATUS "Crow already patched with SO_REUSEPORT")
  return()
endif()

# Insert SO_REUSEPORT after the existing reuse_address set_option block
string(REPLACE
  "acceptor_.raw_acceptor().set_option(Acceptor::reuse_address_option(), ec);"
  "acceptor_.raw_acceptor().set_option(Acceptor::reuse_address_option(), ec);\n\
#ifdef __linux__\n\
            // Allow fast server restart without TIME_WAIT delay\n\
            if (!ec) {\n\
                acceptor_.raw_acceptor().set_option(\n\
                    asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT>(true), ec);\n\
                if (ec) { CROW_LOG_WARNING << \"SO_REUSEPORT not available: \" << ec.message(); ec.clear(); }\n\
            }\n\
#endif"
  CONTENT "${CONTENT}"
)

file(WRITE "${SERVER_H}" "${CONTENT}")
message(STATUS "Patched Crow with SO_REUSEPORT support")

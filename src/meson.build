http_server_sources = ['certificate.cpp',
                       'daemon.cpp',
                       'generate.cpp',
                       'log.cpp',
                       'main.cpp',
                       'mime.cpp',
                       'multipart_wrapper.cpp',
                       'parseqs.cpp',
                       'path.cpp',
                       'server_state.cpp',
                       'session.cpp']

http_server_deps = [boost_dep,
                    openssl_dep,
                    thread_dep,
                    sqlite_dep,
                    inja_dep,
                    multipart_parser_c_dep,
                    tomlplusplus_dep]

http_server_executable = executable('http_server',
                                    sources,
                                    include_directories : inc,
                                    dependencies : http_server_deps)

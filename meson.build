project('http_server',
        'cpp',
        default_options : ['cpp_std=c++2a', 'buildtype=minsize'])

inja_proj = subproject('inja')
inja_dep = inja_proj.get_variable('inja_dep')

multipart_parser_c_proj = subproject('multipart_parser_c')
multipart_parser_c_dep = multipart_parser_c_proj.get_variable('multipart_parser_c_dep')

tomlplusplus_dep = dependency('tomlplusplus')

boost_dep = dependency('boost')
openssl_dep = dependency('openssl')
thread_dep = dependency('threads')
sqlite_dep = dependency('sqlite3')

inc = include_directories('include')

subdir('include')
subdir('src')

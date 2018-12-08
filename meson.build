project('tlwbe', 'c')

mosquittomainloop = subproject('mosquittomainloop')

prog_sqlitegen = find_program(meson.source_root() + '/codegen/sqlitegen.py')
gen_sqlitegen = generator(prog_sqlitegen,
                 output : ['@BASENAME@.sqlite.h'],
                 arguments : ['--input=@INPUT@', '--output=@BUILD_DIR@/@BASENAME@.sqlite.h'])
generated_sqlite = gen_sqlitegen.process('control.h','join.h','uplink.h','downlink.h')


prog_jsongen = find_program(meson.source_root() + '/codegen/jsongen.py')
gen_jsongen = generator(prog_jsongen,
                 output : ['@BASENAME@.json.h'],
                 arguments : ['--input=@INPUT@', '--output=@BUILD_DIR@/@BASENAME@.json.h'])
generated_json = gen_jsongen.process('uplink.h')

src = ['tlwbe.c',
       'crypto.c',
       'join.c',
       'database.c',
       'control.c',
       'utils.c',
       'downlink.c',
       'uplink.c',
       'pktfwdbr.c',
       'packet.c',
       'regional.c',
       'flags.c',
       generated_sqlite,
       generated_json]

deps = [ dependency('glib-2.0'),
         dependency('json-glib-1.0'),
         dependency('sqlite3'),
         dependency('libcrypto'), 
         mosquittomainloop.get_variable('mosquittomainloop_dep')]

executable('tlwbe', src, 
           dependencies : deps, 
           install : true,
           install_dir : 'bin')
project('tlwbe', 'c', default_options: ['c_std=c11'])

cc = meson.get_compiler('c')

add_global_arguments('-ggdb', language : 'c')
add_global_arguments('-Werror=implicit-function-declaration', language : 'c')


conf_data = configuration_data()
conf_data.set('debug', get_option('debug') ? 1 : 0)
conf_data.set('rp_path', join_paths(get_option('prefix'),get_option('datadir'), 'tlwbe', 'regionalparameters.json'))
conf_data.set('state_dir', join_paths(get_option('prefix'),get_option('localstatedir'), 'lib','tlwbe'))
conf_data.set('db_path', join_paths(get_option('prefix'),get_option('localstatedir'), 'lib','tlwbe', 'tlwbe.sqlite'))
configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : conf_data)

mosquitto_glib = subproject('mosquitto_glib')
liblorawan = subproject('liblorawan')

prog_sqlitegen = find_program(meson.source_root() + '/codegen/sqlitegen.py')
gen_sqlitegen = generator(prog_sqlitegen,
                 output : ['@BASENAME@.sqlite.h'],
                 arguments : ['--input=@INPUT@', '--output=@BUILD_DIR@/@BASENAME@.sqlite.h'])
generated_sqlite = gen_sqlitegen.process('control.h','join.h','uplink.h','downlink.h')


prog_jsongen = find_program(meson.source_root() + '/codegen/jsongen.py')
gen_jsongen = generator(prog_jsongen,
                 output : ['@BASENAME@.json.h'],
                 arguments : ['--input=@INPUT@', '--output=@BUILD_DIR@/@BASENAME@.json.h'])
generated_json = gen_jsongen.process('control.h','join.h', 'uplink.h', 
                                     'downlink.h', 'pktfwdbr_txack.h', 'pktfwdbr_rx.h')

src = ['tlwbe.c',
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
       'stats.c',
       'mac.c',
       generated_sqlite,
       generated_json]

deps = [ dependency('glib-2.0'),
         dependency('json-glib-1.0'),
         dependency('sqlite3'),
         liblorawan.get_variable('lorawan_dep'),
         mosquitto_glib.get_variable('mosquitto_glib_dep')]

executable('tlwbe', src, 
           dependencies : deps, 
           install : true,
           install_dir : 'bin')

install_data('regionalparameters.json')

pytest = find_program('pytest-3', required : false)
mosquitto = find_program('mosquitto', '/usr/sbin/mosquitto', required : false)
if pytest.found() and mosquitto.found()
	args = ['--mosquitto_path=' + mosquitto.path(),
	        '--tlwbe_path=' + meson.build_root() + '/tlwbe',
	        '--tlwbe_regionalparameters_path=' +  meson.source_root() + '/regionalparameters.json' ]
	test('control', pytest, timeout : 120,
	     args :  [meson.source_root() + '/tests/test_control.py'] + args,
             is_parallel : false)
	test('downlink', pytest, timeout : 120,
         args :  [meson.source_root() + '/tests/test_downlink.py'] + args,
         is_parallel : false)
endif

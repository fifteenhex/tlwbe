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
codegen = subproject('codegen')


generated_sqlite = codegen.get_variable('gen_sqlitegen').process(meson.current_source_dir() + '/control.h',
                                                                 meson.current_source_dir() + '/join.h',
                                                                 meson.current_source_dir() + '/uplink.h',
                                                                 meson.current_source_dir() + '/downlink.h')
generated_json = codegen.get_variable('gen_jsongen').process(meson.current_source_dir() + '/control.h',
                                                             meson.current_source_dir() + '/join.h',
                                                             meson.current_source_dir() + '/uplink.h',
                                                             meson.current_source_dir() + '/downlink.h',
                                                             meson.current_source_dir() + '/pktfwdbr_txack.h',
                                                             meson.current_source_dir() + '/pktfwdbr_rx.h')
generated_rpc = codegen.get_variable('gen_rpcgen').process(meson.current_source_dir() + '/downlink_rpc.json')

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
       generated_json,
       generated_rpc]

deps = [ dependency('glib-2.0'),
         dependency('json-glib-1.0'),
         dependency('sqlite3'),
         liblorawan.get_variable('lorawan_dep'),
         mosquitto_glib.get_variable('mosquitto_glib_dep'),
         codegen.get_variable('dep')]

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
	        '--tlwbe_regionalparameters_path=' +  meson.source_root() + '/regionalparameters.json',
		    '--capture=no', '--log-level=debug']
	test('control', pytest, timeout : 120,
	     args :  [meson.source_root() + '/tests/test_control.py'] + args,
             is_parallel : false)
	test('uplink', pytest, timeout : 120,
         args :  [meson.source_root() + '/tests/test_uplink.py'] + args,
         is_parallel : false)
	test('downlink', pytest, timeout : 120,
         args :  [meson.source_root() + '/tests/test_downlink.py'] + args,
         is_parallel : false)
endif

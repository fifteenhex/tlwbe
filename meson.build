project('tlwbe', 'c')

mosquittomainloop = subproject('mosquittomainloop')

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
       'flags.c']
deps = [ dependency('glib-2.0'),
         dependency('json-glib-1.0'),
         dependency('sqlite3'),
         dependency('libcrypto'), 
         mosquittomainloop.get_variable('mosquittomainloop_dep')]

executable('tlwbe', src, 
           dependencies : deps, 
           install : true,
           install_dir : 'bin')
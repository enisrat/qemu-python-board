interface_wrap_c = custom_target(
    'interface_wrap.c',
    output : 'interface_wrap.c',
    input : 'interface.i',
    command : ['swig', '-v', '-python', '-o', '@OUTPUT@', '-outdir', meson.current_build_dir(), '-DSWIGWORDSIZE64', '-DCONFIG_INT128', '-I' + meson.global_source_root() + '/include', '@INPUT@' ],
)

system_ss.add(interface_wrap_c)




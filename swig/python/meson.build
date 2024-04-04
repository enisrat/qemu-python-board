
# @TODO Only single target supported right now
target = target_dirs[0]
target_name = target.split('-')[0]
target_inc = 'target' / target_name

interface_wrap_c = custom_target(
    'interface_wrap.c',
    output : 'interface_wrap.c',
    input : 'interface.i',
    command : [ 'swig', '-v', '-python', '-castmode','-o', '@OUTPUT@', '-outdir', meson.current_build_dir(), 
                '-DSWIGWORDSIZE64', '-DCONFIG_INT128', '-DNEED_CPU_H','-DCONFIG_TARGET=@0@-config-target.h'.format(target),
                '-DCONFIG_DEVICES=@0@-config-devices.h'.format(target), '-DTARGET_NAME_@0@'.format(target_name.to_upper()), 
                '-I' + meson.global_source_root() + '/include', 
                '-I' + meson.global_source_root() + '/' + target_inc, 
                '@INPUT@' ],
)

py_ss = ss.source_set()
py_ss.add(interface_wrap_c)
python3 = dependency('python3-embed', required: true)
py_ss.add(python3)
target_modules += { 'swig' : { 'python': py_ss}}





# @TODO Only single target supported right now
target = target_dirs[0]
target_name = target.split('-')[0]
config_target = config_target_mak[target]
target_inc = 'target' / config_target['TARGET_BASE_ARCH']

# Need to process memory.h, because it uses "templated" includes (files ending with ".h.inc")
memory_h_preprocess = custom_target(
    'memory_h_preprocess',
    output: 'memory.h.processed.h',
    input: '../../include/exec/memory.h',
    capture: true,
    feed: true,
    command: [ 'python3', meson.current_source_dir() / 'replace_templated_headers.py', meson.global_source_root()+ '/include']
)

# Just symlink the example.py, so it is in the same folder as SWIG target module
symlink_example = custom_target(
    'symlink_example',
    input: 'example.py',
    output: 'example.py',
    command: ['ln', '-sfr', '@INPUT@', '@OUTPUT@']
)

# The SWIG call. 
# @TODO It has hardcoded config variables: SWIGWORDSIZE64 DCONFIG_INT128
interface_wrap_c = custom_target(
    'interface_wrap.c',
    output : 'interface_wrap.c',
    input : 'interface.i',
    command : [ 'swig', '-v', '-python', '-castmode','-o', '@OUTPUT@', '-outdir', meson.current_build_dir(), 
                '-DSWIGWORDSIZE64', '-DCONFIG_INT128', '-DNEED_CPU_H','-DCONFIG_TARGET=@0@-config-target.h'.format(target),
                '-DCONFIG_DEVICES=@0@-config-devices.h'.format(target), '-DTARGET_NAME_@0@'.format(target_name.to_upper()), 
                '-I' + meson.global_source_root() + '/include', 
                '-I' + meson.global_source_root() + '/' + target_inc, 
                '-I' + meson.current_build_dir(), 
                '@INPUT@' ],
    depends: [memory_h_preprocess, symlink_example]
)

py_ss = ss.source_set()
py_ss.add(interface_wrap_c)
python3 = dependency('python3-embed', required: true)
py_ss.add(python3)
target_modules += { 'swig' : { 'python': py_ss}}




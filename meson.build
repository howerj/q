project('Q-Number (Q16.16, signed) library', 'c',
        version: '0.9.2', license: 'Unlicense',
        default_options: ['c_std=c99'])

q_inc = include_directories('.')

q_c_args = ['-DQVERSION=0x000902']

cc = meson.get_compiler('c')
if cc.has_argument('-fwrapv')
    q_c_args += ['-fwrapv']
endif

q_lib = static_library('q',
    files('q.c'),
    include_directories: q_inc,
    c_args: q_c_args,
)
q_dep = declare_dependency(
    link_whole: q_lib,
    include_directories: q_inc
)

q_expr_exe = executable('expr',
    files('expr.c'),
    dependencies: q_dep
)
test('expr', q_expr_exe)

q_test_exe = executable('t',
    files('t.c'),
    dependencies: q_dep
)
test('t', q_test_exe, args: join_paths(meson.current_source_dir(), 't.q'))

project(
    'ui-tests',
    ['c'],
    version: '0.0.1',
    license: [
        'LGPL-2.1',
    ],
    default_options: [
        'c_std=c11',
    ],
)

am_cflags = [
    '-fstack-protector',
    '-pedantic',
    '-Wstrict-prototypes',
    '-Wundef',
    '-Werror-implicit-function-declaration',
    '-Wformat',
    '-Wformat-security',
    '-Werror=format-security',
    '-Wconversion',
    '-Wunused-variable',
    '-Wunreachable-code',
    '-Wall',
    '-W',
]

add_global_arguments(am_cflags, language: 'c')

# Budgie needs a minimum 3.18 GNOME stack
gnome_minimum_version = '>= 3.18.0'

dep_gtk3 = dependency('gtk+-3.0', version: gnome_minimum_version)


executable(
    'popover-test',
    [
        'src/popover.c',
        'src/popover-manager.c',
        'src/main.c',
    ],
    dependencies: [dep_gtk3],
    install: false,
)

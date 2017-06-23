gnome = import('gnome')

# Generate enums for gobject typing
plugin_enums = gnome.mkenums(
    'budgie-enums',
    sources: 'popover.h',
    c_template: 'budgie-enums.c.template',
    h_template: 'budgie-enums.h.template',
    install_header: false,
)

# Fight meson race conditions..
libenum = static_library(
    'enum',
    plugin_enums,
    dependencies: dep_gtk3,
)

link_libenum = declare_dependency(
    sources: plugin_enums,
    include_directories: include_directories('.'),
)

executable(
    'popover-test',
    [
        'popover.c',
        'popover-manager.c',
        'main.c',
    ],
    dependencies: [dep_gtk3, link_libenum],
    install: false,
)

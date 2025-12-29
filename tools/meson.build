orcc = executable ('orcc', 'orcc.c',
                   gnu_symbol_visibility: 'inlineshidden',
                   install: true,
                   dependencies : [libm, orc_dep])

# Make orcc usable for other subprojects
if meson.can_run_host_binaries()
  meson.override_find_program('orcc', orcc)
endif

orc_bugreport = executable ('orc-bugreport', 'orc-bugreport.c',
                            gnu_symbol_visibility: 'inlineshidden',
                            install: true,
                            dependencies : [orc_dep, orc_test_dep])

if host_os == 'windows'
  orcc_filename = 'orcc.exe'
else
  orcc_filename = 'orcc'
endif

tools_variables = [
  'toolsdir=' + ('${prefix}' / get_option('bindir')),
  'orcc=' + ('${toolsdir}' / orcc_filename),
]

noinst_bins = ['generate-bytecode', 'generate-emulation']
foreach bin : noinst_bins
  executable(bin, bin + '.c',
             gnu_symbol_visibility: 'inlineshidden',
             install: false,
             dependencies: [libm, orc_dep, orc_test_dep])
endforeach

# These are used in doc generation, so we need to keep references to them
generate_md_coverage_table = executable('generate_md_coverage_table', 'generate_md_coverage_table.c',
                                        gnu_symbol_visibility: 'inlineshidden',
                                        install: false,
                                        dependencies: [libm, orc_dep, orc_test_dep])

generate_md_opcodes_table = executable('generate_md_opcodes_table', 'generate_md_opcodes_table.c',
                                       gnu_symbol_visibility: 'inlineshidden',
                                       install: false,
                                       dependencies: [libm, orc_dep, orc_test_dep])

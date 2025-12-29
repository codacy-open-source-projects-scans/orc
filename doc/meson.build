hotdoc = import('hotdoc')

# Configure hotdoc project
hotdoc_subprojects = []

# Get full paths to all C sources and headers for documentation
orc_source_dir = meson.project_source_root() / 'orc'
orc_doc_sources = []

# Add all headers (these are defined in orc/meson.build)
orc_public_headers = [
  'orc.h',
  'orcarm.h',
  'orcavx.h',
  'orcbytecode.h',
  'orccode.h',
  'orccompiler.h',
  'orccpu.h',
  'orcdebug.h',
  'orcexecutor.h',
  'orcfunctions.h',
  'orconce.h',
  'orcopcode.h',
  'orcparse.h',
  'orcpowerpc.h',
  'orcprogram.h',
  'orcrule.h',
  'orcsse.h',
  'orctarget.h',
  'orcutils.h',
  'orcvariable.h',
  'orcx86.h',
  'orcmmx.h',
  'orcneon.h',
  'orcmips.h',
]

# Add C source files (containing gtk-doc comments and SECTION documentation)
orc_doc_c_sources = [
  'orc.c',
  'orcarm.c',
  'orcavx.c',
  'orccompiler.c',
  'orcdebug.c',
  'orcexecutor.c',
  'orcmmx.c',
  'orcopcode.c',
  'orcparse.c',
  'orcpowerpc.c',
  'orcprogram.c',
  'orcrule.c',
  'orcsse.c',
  'orcutils.c',
  'orcx86.c',
]

# Build list of sources for documentation
# For smart indexing: only headers in c_sources, .c files for comment extraction
orc_doc_c_sources_paths = []
foreach c : orc_doc_c_sources
  orc_doc_c_sources_paths += orc_source_dir / c
endforeach

orc_doc_headers_paths = []
foreach h : orc_public_headers
  orc_doc_headers_paths += orc_source_dir / h
endforeach

# Generate markdown tables for opcodes documentation
opcode_table_md = custom_target('opcode_table_md',
  output: 'opcode_table.md',
  command: [generate_md_opcodes_table],
  capture: true,
  build_by_default: false,
)

rule_coverage_table_md = custom_target('rule_coverage_table_md',
  output: 'rule_coverage_table.md',
  command: [generate_md_coverage_table],
  capture: true,
  build_by_default: false,
)

# Generate opcodes.md from template by replacing placeholders with generated tables
opcodes_md = custom_target('opcodes_md',
  input: 'opcodes.md.in',
  output: 'opcodes.md',
  command: [
    'sed',
    '-e', '/@OPCODE_TABLE@/ {',
    '-e', 'r ' + opcode_table_md.full_path(),
    '-e', 'd',
    '-e', '}',
    '-e', '/@RULE_COVERAGE_TABLE@/ {',
    '-e', 'r ' + rule_coverage_table_md.full_path(),
    '-e', 'd',
    '-e', '}',
    '@INPUT@'
  ],
  capture: true,
  depends: [opcode_table_md, rule_coverage_table_md],
  build_by_default: true,
  install: false,
)

orc_doc = hotdoc.generate_doc('orc',
  project_version: meson.project_version(),
  sitemap: 'sitemap.txt',
  index: 'index.md',
  install: true,
  extra_assets: [],
  include_paths: [
    meson.current_source_dir(),
    meson.current_build_dir(),
  ],
  depends: [opcodes_md],
  c_sources: orc_doc_headers_paths + orc_doc_c_sources_paths,
  c_source_roots: [orc_source_dir],
  c_smart_index: true,
  c_include_directories: [
    orc_source_dir,
    meson.project_source_root(),
  ],
  dependencies: [orc_dep],
  subprojects: hotdoc_subprojects,
)

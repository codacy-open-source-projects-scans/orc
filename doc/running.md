# Running Orc Applications

At runtime several environment variables can be used to control ORC.

## ORC_DEBUG

This environment variable can be set to a number indicating the level for debug logging. A value of 0 (default) results in no debug logging. The higher the value to more detail is logged.

## ORC_CODE

This variable can be set to a comma separated list of flags to control the code selection and execution. Supported values are: backup, emulate and debug. The value 'backup' would instruct ORC to select the C based backup functions. Selecting 'emulate' will run the ORC code through an interpreter. Using 'debug' enables debuggers such as gdb to create useful backtraces from ORC-generated code

## ORC_TARGET

This environment variable can be used to override the default target selection and force a specific target such as "sse" or "avx". This can be useful for benchmarking purposes, e.g. to compare SSE vs. AVX targets, or to work around bugs. Available since Orc 0.4.35.

#ifndef CPROVER_GOTO_PROGRAMS_RACE_DETECTION_H
#define CPROVER_GOTO_PROGRAMS_RACE_DETECTION_H

#include <goto-programs/goto_functions.h>
#include <goto-programs/goto_program.h>
#include <pointer-analysis/value_sets.h>

void add_race_assertions(
  value_setst &value_sets,
  contextt &context,
  goto_programt &goto_program);

void add_race_assertions(
  value_setst &value_sets,
  contextt &context,
  goto_functionst &goto_functions);

// for Standard and CUDA library. (A special case: "exception" library's file name is "")
static const std::unordered_set<std::string> ignored_library = {
  "pthread_lib.c",
  "builtin_libs.c",
  "stdlib.c",
  "string.c",
  "io.c",
  "",
  "cuda_runtime_api.h",
  "call_kernel.h",
  "device_launch_parameters.h",
  "sm_atomic_functions.h",
  "vector_types.h",
  "driver_types.h"};

#endif

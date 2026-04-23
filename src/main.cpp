/// \file
/// Wildframe entry point. A 3-line wrapper around `Run()` so TB-09's
/// end-to-end smoke test can invoke the CLI in-process without
/// spawning a subprocess (TB-09, TB-01). Real behavior lives in
/// `run.cpp`.

#include <iostream>

#include "run.hpp"

int main(int argc, char* argv[]) {
  return wildframe::cli::Run(argc, argv, std::cerr);
}

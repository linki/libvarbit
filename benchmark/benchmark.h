#ifndef BENCHMARK_BENCHMARK_H_
#define BENCHMARK_BENCHMARK_H_

#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <papi.h>
#include <iostream>

#ifndef PAPI_COUNTERS
#define PAPI_COUNTERS {PAPI_TOT_CYC, PAPI_TOT_INS}
#endif  // PAPI_COUNTERS
#ifndef NUM_PAPI_COUNTERS
#define NUM_PAPI_COUNTERS 2
#endif  // NUM_PAPI_COUNTERS
const int num_papi_counters = NUM_PAPI_COUNTERS;
// Must not be const, because PAPI's interface is weird.
int papi_counters[NUM_PAPI_COUNTERS] = PAPI_COUNTERS;

// Simple Configuration class handling command lines parameters and benchmark
// global setup. Aways create an instance of this in your main:
// Configuration x = Config(argc, argv);
// Subsequent argumentless calls to Config() will return the same Configuration.
class Configuration {
 public:
  Configuration(int argc, char **argv);
  uint64_t num_elements() const { return num_elements_; }
 private:
  uint64_t num_elements_;
};

Configuration::Configuration(int argc, char **argv) {
  if (!mlockall(MCL_CURRENT & MCL_FUTURE)) {
    std::cerr << "mlockall failed: Couldn't lock memory." << std::endl;
    exit(1);
  }
  const int status = PAPI_start_counters(papi_counters, num_papi_counters);
  if (status != PAPI_OK) {
    std::cout << "Can't start PAPI Counters (" << status << ")." << std::endl;
    exit(1);
  }
  if (argc != 2) {
    std::cout << "Please specify the number of elements as a parameter."
              << std::endl;
    exit(1);
  }
  num_elements_ = atoi(argv[1]) * 1024 * 1024;
}

Configuration Config(int argc = 0, char **argv = NULL) {
  static Configuration instance(argc, argv);
  return instance;
}

// Sample usage to measure the function int foo(int) with int bar as parameter:
// Benchmark<int, int> benchmark(foo);
// int result = benchmark.run(bar);
// Make sure to initialize Configuration before!
template <typename Input, typename Result>
class Benchmark {
 public:
  Benchmark(Result(*function)(Input), const char* function_name,
            const char* data_structure_name);
  Result run(Input input, const int bit_width);
 private:
  Result(*function_)(Input);
  const char *function_name_;
  const char *data_structure_name_;
  long long int papi_values_[num_papi_counters]; // NOLINT PAPI interface again
};

template<typename Input, typename Result>
Benchmark<Input, Result>::Benchmark(Result(*function)(Input),
                                    const char *data_structure_name,
                                    const char *function_name)
    : function_(function),
      function_name_(function_name),
      data_structure_name_(data_structure_name) {
    std::cout << "#function,data_structure,bit_width";
    for (int i = 0; i < num_papi_counters; ++i) {
      char event_name[PAPI_MAX_STR_LEN];
      PAPI_event_code_to_name(papi_counters[i], event_name);
      std::cout << ',' << event_name;
    }
    std::cout << std::endl;
  }

template<typename Input, typename Result>
Result Benchmark<Input, Result>::run(Input input, const int bit_width) {
  PAPI_read_counters(papi_values_, num_papi_counters);
  Result result = function_(input);
  PAPI_read_counters(papi_values_, num_papi_counters);
  std::cout << function_name_ << ',' << data_structure_name_ << ','
            << bit_width;
  for (int i = 0; i < num_papi_counters; ++i) {
    std::cout << ',' << papi_values_[i];
  }
  std::cout << std::endl;
  return result;
}
#endif  // BENCHMARK_BENCHMARK_H_

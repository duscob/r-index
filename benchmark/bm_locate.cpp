//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 8/28/20.
//

#include <iostream>
#include <fstream>

#include <benchmark/benchmark.h>

#include <gflags/gflags.h>

#include <sdsl/config.hpp>
#include <sdsl/construct.hpp>
#include <internal/r_index.hpp>

DEFINE_string(patterns, "", "Patterns file. (MANDATORY)");
DEFINE_string(data_dir, "./", "Data directory.");
DEFINE_string(data_name, "data", "Data file basename.");
DEFINE_bool(print_result, false, "Execute benchmark that print results per index.");
DEFINE_bool(rebuild, false, "Rebuild index.");
DEFINE_bool(sais, true, "SE_SAIS or LIBDIVSUFSORT algorithm for Suffix Array construction.");

const std::string KEY_R_INDEX = "ri";

void SetupDefaultCounters(benchmark::State &t_state) {
  t_state.counters["Collection_Size(bytes)"] = 0;
  t_state.counters["Size(bytes)"] = 0;
  t_state.counters["Bits_x_Symbol"] = 0;
  t_state.counters["Patterns"] = 0;
  t_state.counters["Time_x_Pattern"] = 0;
  t_state.counters["Occurrences"] = 0;
  t_state.counters["Time_x_Occurrence"] = 0;
}

// Benchmark Warm-up
static void BM_WarmUp(benchmark::State &t_state) {
  for (auto _ : t_state) {
    std::vector<int> empty_vector(1000000, 0);
  }

  SetupDefaultCounters(t_state);
}
BENCHMARK(BM_WarmUp);

auto BM_QueryLocate =
    [](benchmark::State &t_state, auto &t_idx, const auto &t_patterns, auto t_seq_size) {
      std::size_t n_occs = 0;

      for (auto _ : t_state) {
        n_occs = 0;
        for (auto pattern: t_patterns) {
          auto occs = t_idx.first->locate_all(pattern);
          n_occs += occs.size();
        }
      }

      SetupDefaultCounters(t_state);
      t_state.counters["Collection_Size(bytes)"] = t_seq_size;
      t_state.counters["Size(bytes)"] = t_idx.second;
      t_state.counters["Bits_x_Symbol"] = t_idx.second * 8.0 / t_seq_size;
      t_state.counters["Patterns"] = t_patterns.size();
      t_state.counters["Time_x_Pattern"] = benchmark::Counter(
          t_patterns.size(), benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert);
      t_state.counters["Occurrences"] = n_occs;
      t_state.counters["Time_x_Occurrence"] = benchmark::Counter(
          n_occs, benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert);
    };

auto BM_PrintQueryLocate =
    [](benchmark::State &t_state, const auto &t_idx_name, const auto &t_idx, const auto &t_patterns, auto t_seq_size) {
      std::string idx_name = t_idx_name;
      replace(idx_name.begin(), idx_name.end(), '/', '_');
      std::string output_filename = "result-" + idx_name + ".txt";

      std::size_t n_occs = 0;

      for (auto _ : t_state) {
        std::ofstream out(output_filename);
        n_occs = 0;
        for (auto pattern: t_patterns) {
          out << pattern << std::endl;
          auto occs = t_idx.first->locate_all(pattern);
          n_occs += occs.size();

          sort(occs.begin(), occs.end());
          for (const auto &item  : occs) {
            out << "  " << item << std::endl;
          }
        }
      }

      SetupDefaultCounters(t_state);
      t_state.counters["Collection_Size(bytes)"] = t_seq_size;
      t_state.counters["Size(bytes)"] = t_idx.second;
      t_state.counters["Bits_x_Symbol"] = t_idx.second * 8.0 / t_seq_size;
      t_state.counters["Patterns"] = t_patterns.size();
      t_state.counters["Time_x_Pattern"] = benchmark::Counter(
          t_patterns.size(), benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert);
      t_state.counters["Occurrences"] = n_occs;
      t_state.counters["Time_x_Occurrence"] = benchmark::Counter(
          n_occs, benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert);
    };

auto BM_AccessF = [](benchmark::State &_state, const auto &t_f, std::size_t t_n) {
  std::cout << t_n << std::endl;
  for (auto _ : _state) {
    for (std::size_t j = 0; j < 10; ++j) {
      for (int i = 0; i < t_n; ++i) {
        auto v = t_f[i];
      }
    }
  }

  SetupDefaultCounters(_state);
};

auto BM_GetLF = [](benchmark::State &_state, const auto &t_idx, std::size_t t_n, std::size_t t_n_c) {
  for (auto _ : _state) {
    for (int i = 0; i < t_n; i += 10000) {
      std::pair<std::size_t, std::size_t> range = {i, t_n - 100};
      for (int k = 0; k < t_n_c; ++k) {
        auto v = t_idx.first->LF(range, k);
      }
    }
  }

  SetupDefaultCounters(_state);
};

int main(int argc, char *argv[]) {
  gflags::AllowCommandLineReparsing();
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  if (FLAGS_patterns.empty() || FLAGS_data_name.empty() || FLAGS_data_dir.empty()) {
    std::cerr << "Command-line error!!!" << std::endl;
    return 1;
  }

  // Query patterns
  std::vector<std::string> patterns;
  {
    std::ifstream pattern_file(FLAGS_patterns.c_str(), std::ios_base::binary);
    if (!pattern_file) {
      std::cerr << "ERROR: Failed to open patterns file!" << std::endl;
      return 3;
    }

    std::string buf;
    while (std::getline(pattern_file, buf)) {
      if (buf.empty())
        continue;

      patterns.emplace_back(buf);
    }
    pattern_file.close();
  }

  // Create indexes
  sdsl::cache_config config(true, FLAGS_data_dir, FLAGS_data_name);
  ri::r_index<> r_idx;
  std::pair<ri::r_index<> *, std::size_t> index;

  if (!cache_file_exists(KEY_R_INDEX, config) || FLAGS_rebuild) {
    construct_config::byte_algo_sa = FLAGS_sais ? SE_SAIS
                                                : LIBDIVSUFSORT; // or LIBDIVSUFSORT for less space-efficient but faster construction

    string data_path = FLAGS_data_dir + "/" + FLAGS_data_name;

    std::string input;
    {
      std::ifstream fs(data_path);
      std::stringstream buffer;
      buffer << fs.rdbuf();

      input = buffer.str();
    }

    std::replace(input.begin(), input.end(), '\0', '\2');
    r_idx = ri::r_index<>(input, FLAGS_sais);

    sdsl::store_to_cache(r_idx, KEY_R_INDEX, config);

  } else {
    sdsl::load_from_cache(r_idx, KEY_R_INDEX, config);
  }

  index.first = &r_idx;
  index.second = sdsl::size_in_bytes(r_idx);


  // Indexes
  std::string index_name = "r-index";
  benchmark::RegisterBenchmark(index_name.c_str(), BM_QueryLocate, index, patterns, index.first->text_size());

  std::string print_bm_prefix = "Print-";
  if (FLAGS_print_result) {
    auto print_bm_name = print_bm_prefix + index_name;
    benchmark::RegisterBenchmark(
        print_bm_name.c_str(), BM_PrintQueryLocate, index_name, index, patterns, index.first->text_size());
  }

//  benchmark::RegisterBenchmark("BM_AccessF", BM_AccessF, r_idx.F, r_idx.F.size());

//  benchmark::RegisterBenchmark("BM_GetLF",
//                               BM_GetLF,
//                               index,
//                               r_idx.bwt.size(),
//                               r_idx.F.size());


  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

  return 0;
}
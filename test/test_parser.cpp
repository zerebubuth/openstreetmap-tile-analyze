#include "types.hpp"
#include "parser.hpp"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <vector>

namespace fs = boost::filesystem;

namespace {

struct tmp_dir {
  const fs::path m_path;

  tmp_dir() : m_path(fs::temp_directory_path() / fs::unique_path()) {
    if (!fs::create_directories(m_path)) {
      throw std::runtime_error("Unable to create temporary directory.");
    }
  }

  ~tmp_dir() {
    try {
      fs::remove_all(m_path);

    } catch (const std::exception &e) {
      std::cerr << "Unable to remove temporary directory: " << e.what() << "\n";
    }
  }
};

void run_test(const fs::path &input_file) {
  tmp_dir output_dir;
  const size_t max_size = 16 * 1024 * 1024;

  std::list<fs::path> dat_files = file_parser(input_file, output_dir.m_path, max_size);

  for (const auto &dat_file : dat_files) {
    if (!fs::exists(dat_file)) {
      throw std::runtime_error((boost::format("Data file %1% was supposed to exist "
                                              "when processing input %2%.")
                                % dat_file % input_file).str());
    }
    if (fs::file_size(dat_file) == 0) {
      throw std::runtime_error((boost::format("Data file %1% should have >0 size "
                                              "when processing input %2%.")
                                % dat_file % input_file).str());
    }
  }
}

} // anonymous namespace

int main() {
  bool failed = true;
  std::vector<fs::path> files = {
    "test/data/not_exist.log.xz",
    "test/data/empty.log.xz",
    "test/data/corrupt.log.xz",
    "test/data/unparseable.log.xz",
    "test/data/simple.log.xz"
  };

  try {
    for (const auto &input_file : files) {
      run_test(input_file);
    }

    failed = false;

  } catch (const std::exception &e) {
    std::cerr << "Test failed: " << e.what() << "\n";
  } catch (...) {
    std::cerr << "Test failed: UNKNOWN EXCEPTION.\n";
  }

  return (failed ? 1 : 0);
}

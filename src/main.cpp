#include "types.hpp"
#include "parser.hpp"
#include "datfile.hpp"

#include <boost/filesystem.hpp>

#include <future>
#include <list>
#include <fstream>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace fs = boost::filesystem;

std::list<fs::path> parse_logfiles(std::list<fs::path> input_files,
                                   fs::path output_dir,
                                   size_t max_size) {
  std::list<std::future<std::list<fs::path> > > futures;
  std::list<fs::path> dat_files;

  for (fs::path input_file : input_files) {
    futures.emplace_back(std::async(std::launch::async, file_parser, input_file, output_dir, max_size));
  }

  for (auto &future : futures) {
     dat_files.splice(dat_files.end(), future.get());
  }

  return dat_files;
}

std::list<datfile> sort_datfiles(const std::list<fs::path> &dat_files) {
   std::list<std::future<datfile> > futures;
   std::list<datfile> files;

   for (fs::path file : dat_files) {
     futures.push_back(std::async(std::launch::async, sort_datfile, file));
   }

   for (std::future<datfile> &future : futures) {
      files.emplace_back(future.get());
   }

   return files;
}

void output_counts(std::list<datfile> &files) {
   std::vector<datfile> vfiles;
   while (!files.empty()) {
      vfiles.emplace_back(std::move(files.front()));
      files.pop_front();
   }

   const size_t nfiles = vfiles.size();
   bool any_left = false;
   std::vector<datfile::iterator> vitrs, vends;
   for (size_t i = 0; i < nfiles; ++i) {
      datfile::iterator itr = vfiles[i].begin();
      datfile::iterator end = vfiles[i].end();
      vitrs.push_back(itr);
      vends.push_back(end);
      if (itr != end) { any_left = true; }
   }

   std::ofstream output("tiles.txt");
   log_line last_line;
   size_t line_count = 0, ip_addresses = 0;
   while (any_left) {
      any_left = false;
      log_line line;
      size_t idx = 0;

      for (size_t i = 0; i < nfiles; ++i) {
         if (vitrs[i] != vends[i]) {
            if (any_left) {
               if (*vitrs[i] < line) { idx = i; line = *vitrs[i]; }
            } else {
               idx = i;
               line = *vitrs[i];
            }
            any_left = true;
         }
      }

      if (any_left) {
         if (last_line != line) {
            if ((last_line.z != line.z) ||
                (last_line.x != line.x) ||
                (last_line.y != line.y)) {
               // output cumulative count and reset
               if ((line_count >= 10) && (ip_addresses >= 3)) {
                  output << last_line.z << "/"
                         << last_line.x << "/"
                         << last_line.y << " "
                         << line_count << "\n";
               }

               ip_addresses = 1;
               line_count = 0;

            } else if (last_line.ip_addr != line.ip_addr) {
               ++ip_addresses;
            }

            last_line = line;
         }
         ++line_count;
         vitrs[idx]++;
      }
   }
   if ((line_count >= 10) && (ip_addresses >= 3)) {
      output << last_line.z << "/"
             << last_line.x << "/"
             << last_line.y << " "
             << line_count << std::endl;
   }
}

int main(int argc, char *argv[]) {
   std::list<fs::path> input_files;
   fs::path base_dir("db");
   size_t max_size = 16 * 1024 * 1024;

   for (int i = 1; i < argc; ++i) {
      input_files.push_back(fs::path(argv[i]));
   }

   std::list<fs::path> dat_files = parse_logfiles(input_files, base_dir, max_size);

   std::list<datfile> files = sort_datfiles(dat_files);

   output_counts(files);

   return 0;
}

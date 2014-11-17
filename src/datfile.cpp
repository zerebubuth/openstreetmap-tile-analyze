#include "datfile.hpp"

#include <boost/format.hpp>

#include <stdexcept>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace fs = boost::filesystem;

datfile::datfile(boost::filesystem::path file) 
   : m_ptr(nullptr)
   , m_size(0) {

   size_t file_size = fs::file_size(file);

   int fd = open(file.c_str(), O_RDWR);
   if (fd == -1) {
      throw std::runtime_error((boost::format("Unable to open %1%: %2%")
                                % file % strerror(errno)).str());
   }

   void *ptr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (ptr == MAP_FAILED) {
      throw std::runtime_error((boost::format("Unable to mmap %1%: %2%")
                                % file % strerror(errno)).str());
   }
   close(fd);

   m_ptr = ptr;
   m_size = file_size;
}

datfile::datfile(datfile &&d) 
   : m_ptr(d.m_ptr)
   , m_size(d.m_size) {
   d.m_ptr = nullptr;
   d.m_size = 0;
}

datfile::~datfile() {
   munmap(m_ptr, m_size);
}

datfile::const_iterator datfile::begin() const {
   return (const_iterator)m_ptr;
}

datfile::const_iterator datfile::end() const {
   return begin() + (m_size / sizeof(log_line));
}

datfile::iterator datfile::begin() {
   return (iterator)m_ptr;
}

datfile::iterator datfile::end() {
   return begin() + (m_size / sizeof(log_line));
}

void datfile::sort() {
   std::sort(begin(), end());
}

datfile sort_datfile(boost::filesystem::path file) {
   datfile d(file);
   d.sort();
   return d;
}

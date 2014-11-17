#ifndef SQUID_ANALYZE_DATFILE_HPP
#define SQUID_ANALYZE_DATFILE_HPP

#include "types.hpp"
#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>

/**
 */
struct datfile : public boost::noncopyable {
   typedef const log_line* const_iterator;
   typedef log_line* iterator;

   datfile(boost::filesystem::path file);
   datfile(datfile &&);
   ~datfile();

   iterator begin();
   iterator end();
   const_iterator begin() const;
   const_iterator end() const;

   void sort();

private:
   void *m_ptr;
   size_t m_size;
};

datfile sort_datfile(boost::filesystem::path file);

#endif /* SQUID_ANALYZE_DATFILE_HPP */

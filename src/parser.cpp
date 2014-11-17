#include "parser.hpp"
#include "types.hpp"

#include <boost/exception/all.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <boost/filesystem.hpp>

#include <stdexcept>
#include <string>
#include <sstream>
#include <fstream>

#include <cstdio>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace fs = boost::filesystem;

struct tag_copy_header;
struct tag_leveldb_status;

typedef boost::error_info<tag_copy_header, std::string>    copy_header;
typedef boost::error_info<tag_leveldb_status, std::string> leveldb_status;

struct popen_error : public boost::exception, std::exception {};
struct fread_error : public boost::exception, std::exception {};
struct early_termination_error : public boost::exception, std::exception {};
struct copy_header_parse_error : public boost::exception, std::exception {};
struct leveldb_error : public boost::exception, std::exception {};

typedef boost::shared_ptr<FILE> pipe_ptr;

static void pipe_closer(FILE *fh) {
  if (fh != NULL) {
    if (pclose(fh) == -1) {
      std::cerr << "ERROR while closing popen." << std::endl;
      abort();
    }
  }
}

struct process 
  : public boost::noncopyable {
  explicit process(const std::string &cmd) 
    : m_fh(popen(cmd.c_str(), "r"), &pipe_closer) {
    if (!m_fh) {
      BOOST_THROW_EXCEPTION(popen_error() << boost::errinfo_file_name(cmd));
    }
  }

  ~process() {
  }

  size_t read(char *buf, size_t len) {
    size_t n = fread(buf, 1, len, m_fh.get());
    if (ferror(m_fh.get()) != 0) {
      boost::weak_ptr<FILE> fh(m_fh);
      BOOST_THROW_EXCEPTION(fread_error() << boost::errinfo_file_handle(fh));
    }
    return n;
  }
    
private:
  pipe_ptr m_fh;
};

template <typename T>
struct to_line_filter 
  : public boost::noncopyable {
  to_line_filter(T &source, size_t buffer_size) 
    : m_source(source), 
      m_buffer(buffer_size, '\0'),
      m_buffer_pos(m_buffer.begin()),
      m_buffer_end(m_buffer_pos) {
  }

  ~to_line_filter() {
  }

  size_t read(std::string &line) {
    line.clear();
    std::string::iterator begin_pos = m_buffer_pos;
    char c = '\0';

    do {
      if (m_buffer_pos == m_buffer_end) {
        line.append(begin_pos, m_buffer_pos);
        if (refill() == 0) {
          return 0;
        }
        begin_pos = m_buffer_pos;
      }

      c = *m_buffer_pos;
      if (c != '\n') {
        ++m_buffer_pos;
      }
    } while (c != '\n');

    line.append(begin_pos, m_buffer_pos);
    ++m_buffer_pos;

    return 1;
  }

private:
  size_t refill() {
    size_t bytes = 0;
    while (bytes < m_buffer.size()) {
      size_t len = m_source.read(&m_buffer[bytes], m_buffer.size() - bytes);
      if (len == 0) {
        break;
      }
      bytes += len;
    }
    m_buffer_pos = m_buffer.begin();
    m_buffer_end = m_buffer.begin() + bytes;
    return bytes;
  }

  T &m_source;
  std::string m_buffer;
  std::string::iterator m_buffer_pos, m_buffer_end;
};

uint32_t mk_ip_addr(const boost::fusion::vector4<int, int, int, int> &v) {
  using boost::fusion::at_c;

  return (((uint32_t(at_c<0>(v)) * 256u +
            uint32_t(at_c<1>(v))) * 256u + 
           uint32_t(at_c<2>(v))) * 256u +
          uint32_t(at_c<3>(v)));
}

struct ip_addr_for_qi {
  int a, b, c, d;
};

struct zxy_t {
  int32_t z, x, y;
};

struct log_line_for_qi {
  zxy_t zxy;
  ip_addr_for_qi ip_addr;
};

inline uint64_t interleave(uint64_t x) {
  x = (x | (x << 16)) & 0x0000ffff0000fffful;
  x = (x | (x <<  8)) & 0x00ff00ff00ff00fful;
  x = (x | (x <<  4)) & 0x0f0f0f0f0f0f0f0ful;
  x = (x | (x <<  2)) & 0x3333333333333333ul;
  x = (x | (x <<  1)) & 0x5555555555555555ul;
  return x;
}

inline uint64_t morton(uint64_t x, uint64_t y) {
  return (interleave(x) << 1) | interleave(y);
}

inline uint64_t unmorton(uint64_t x) {
  x = x & 0x5555555555555555ul;
  x = (x | (x >>  1)) & 0x3333333333333333ul;
  x = (x | (x >>  2)) & 0x0f0f0f0f0f0f0f0ful;
  x = (x | (x >>  4)) & 0x00ff00ff00ff00fful;
  x = (x | (x >>  8)) & 0x0000ffff0000fffful;
  x = (x | (x >> 16)) & 0x00000000fffffffful;
  return x;
}
BOOST_FUSION_ADAPT_STRUCT(
  log_line_for_qi,
  (zxy_t, zxy)
  (ip_addr_for_qi, ip_addr)
  );

BOOST_FUSION_ADAPT_STRUCT(
  ip_addr_for_qi,
  (int, a)
  (int, b)
  (int, c)
  (int, d)
  );

BOOST_FUSION_ADAPT_STRUCT(
  zxy_t,
  (int32_t, z)
  (int32_t, x)
  (int32_t, y)
  );

template <typename Iterator>
struct log_line_parser : qi::grammar<Iterator, log_line_for_qi()> {
  log_line_parser() : log_line_parser::base_type(top) {
    using qi::omit;
    using qi::double_;
    using qi::no_skip;
    using qi::lit;
    using qi::int_;
    using qi::_val;
    using qi::_1;
    using qi::_2;
    using qi::_3;
    using qi::_4;
    using qi::lexeme;
    using qi::char_;

    top = omit[double_] >> lit(" ") // time
      >> path >> lit(" ")
      >> omit[quoted_string] >> lit(" ") // etag?
      >> ip_addr >> lit(" ")
      >> omit[quoted_string] // user agent
      ;

    path = (
      lit("/") >> int_ >> 
      lit("/") >> int_ >> 
      lit("/") >> int_ >> 
      lit(".png")
      );

    ip_addr = (
      int_ >> lit(".") >>
      int_ >> lit(".") >>
      int_ >> lit(".") >>
      int_
      );

    quoted_string = lit("\"") >> *((char_ - '"' - '\\') || ('\\' >> char_)) >> lit("\"");

    /*
    qi::debug(top);
    qi::debug(path);
    qi::debug(ip_addr);
    qi::debug(quoted_string);

    BOOST_SPIRIT_DEBUG_NODE(top);
    BOOST_SPIRIT_DEBUG_NODE(path);
    BOOST_SPIRIT_DEBUG_NODE(ip_addr);
    BOOST_SPIRIT_DEBUG_NODE(quoted_string);
    */
  }

  qi::rule<Iterator, log_line_for_qi()> top;
  qi::rule<Iterator, zxy_t()> path;
  qi::rule<Iterator, ip_addr_for_qi()> ip_addr;
  qi::rule<Iterator, std::string()> quoted_string;
};

std::list<fs::path> file_parser(fs::path input_file,
                                fs::path output_dir,
                                size_t max_size) {
  const size_t buffer_size = 1024 * 64;
  const std::string stem = input_file.stem().native();
  size_t part = 0, current_size = 0;
  std::list<fs::path> output_files;

  fs::path output_file = output_dir / (boost::format("%s_%06d.dat") % stem % part).str();
  std::ofstream output(output_file.c_str());
  output_files.push_back(output_file);

  process proc((boost::format("xzcat %1%") % input_file).str());
  to_line_filter<process> to_line(proc, buffer_size);
  std::string str_line;
  log_line_parser<std::string::iterator> parser;
  log_line_for_qi line_for_qi;
  log_line line;
  size_t total = 0, unparsed = 0;

  while (to_line.read(str_line)) {
    std::string::iterator begin = str_line.begin();
    const std::string::iterator end = str_line.end();

    if (qi::parse(begin, end, parser, line_for_qi)) {
      if ((line_for_qi.zxy.z >= 0) && (line_for_qi.zxy.z <= 19)) {
        const int max_coord = 1 << line_for_qi.zxy.z;
        if (((line_for_qi.zxy.x >= 0) && (line_for_qi.zxy.x < max_coord)) &&
            ((line_for_qi.zxy.y >= 0) && (line_for_qi.zxy.y < max_coord))) {
          line.z = line_for_qi.zxy.z;
          line.x = line_for_qi.zxy.x;
          line.y = line_for_qi.zxy.y;
          line.ip_addr = mk_ip_addr(line_for_qi.ip_addr);
          
          output.write((const char *)(&line), sizeof line);
          current_size += sizeof line;

          if (current_size >= max_size) {
             output.close();
             ++part;
             current_size = 0;

             output_file = output_dir / (boost::format("%s_%06d.dat") % stem % part).str();
             output.open(output_file.c_str());
             output_files.push_back(output_file);
          }
        }
      }
    } else {
      ++unparsed;
    }

    ++total;
  }

  std::ostringstream out;
  out << "[" << input_file << "]: failed to parse " << unparsed
      << " of " << total << " lines.\n";
  std::string str(out.str());
  std::cout.write(str.data(), str.size());

  return output_files;
}

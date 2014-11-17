#ifndef SQUID_ANALYZE_PARSER_HPP
#define SQUID_ANALYZE_PARSER_HPP

#include <boost/filesystem.hpp>
#include <list>

/**
 * parse the squid logfile given in `input_file`, using `output_dir` to put
 * the output files (each kept approximately to `max_size`) and returning
 * their names in list.
 */
std::list<boost::filesystem::path> file_parser(boost::filesystem::path input_file,
                                               boost::filesystem::path output_dir,
                                               size_t max_size);

#endif /* SQUID_ANALYZE_PARSER_HPP */

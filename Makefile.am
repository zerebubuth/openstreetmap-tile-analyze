AM_CXXFLAGS = @BOOST_CPPFLAGS@
AM_LDFLAGS = @BOOST_LDFLAGS@

bin_PROGRAMS = openstreetmap-tile-analyze

noinst_LIBRARIES = libopenstreetmap-tile-analyze.a

libopenstreetmap_tile_analyze_a_SOURCES = src/parser.cpp src/datfile.cpp src/types.cpp

openstreetmap_tile_analyze_SOURCES = src/main.cpp
openstreetmap_tile_analyze_LDADD = \
	libopenstreetmap-tile-analyze.a \
	@BOOST_PROGRAM_OPTIONS_LIB@ \
	@BOOST_FILESYSTEM_LIB@ \
	@BOOST_SYSTEM_LIB@

check_PROGRAMS = \
	test/test_parser

test_test_parser_SOURCES = test/test_parser.cpp
test_test_parser_LDADD = \
	libopenstreetmap-tile-analyze.a \
	@BOOST_PROGRAM_OPTIONS_LIB@ \
	@BOOST_FILESYSTEM_LIB@ \
	@BOOST_SYSTEM_LIB@

TESTS = $(check_PROGRAMS)

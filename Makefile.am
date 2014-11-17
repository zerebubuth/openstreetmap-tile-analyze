bin_PROGRAMS = openstreetmap-tile-analyze

openstreetmap_tile_analyze_SOURCES = src/main.cpp src/parser.cpp src/datfile.cpp src/types.cpp
openstreetmap_tile_analyze_CXXFLAGS = @BOOST_CPPFLAGS@
openstreetmap_tile_analyze_LDFLAGS = @BOOST_LDFLAGS@
openstreetmap_tile_analyze_LDADD = \
	@BOOST_PROGRAM_OPTIONS_LIB@ \
	@BOOST_FILESYSTEM_LIB@ \
	@BOOST_SYSTEM_LIB@

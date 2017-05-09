FLAGS=-std=c++14 -pthread -Wall -Wextra -Weffc++ -Wconversion -Wpedantic -pedantic

all: | doc test

clean: clean-test clean-doc

clean-doc:
	-rm -rf doc
	
clean-test:
	-rm -rf test/*.run
	
doc: clean-doc
	doxygen

doc-internal: clean-doc
	cp Doxyfile Doxyfile.internal
	echo "INTERNAL_DOCS = YES" >> Doxyfile.internal
	echo "EXTRACT_PRIVATE = YES" >> Doxyfile.internal
	doxygen Doxyfile.internal
	
test: ${patsubst %.cpp,%.run,${wildcard test/*.cpp}}

test/%.run: test/%.cpp test/test_helper.hpp include/hash_map.hpp
	$(CXX) $(FLAGS) -o $@ $<
	$@
  
.PHONY: all clean clean-doc clean-test doc doc-internal test

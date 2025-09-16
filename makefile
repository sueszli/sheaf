DOCKER_RUN = docker run --rm -v $(PWD):/workspace main sh -c

.PHONY: run
run:
	scons -j$$(nproc) run
	@scons -c -s

.PHONY: memcheck
memcheck:
	scons -j$$(nproc)
	scons VALGRIND=1 valgrind
	@scons -c -s

# .PHONY: test
# test:
# 	$(DOCKER_RUN) 'rm -rf build && mkdir -p build && cd build && cmake -DBUILD_TESTS=ON .. && cmake --build . -j$$(nproc)'
# 	$(DOCKER_RUN) 'cd build && ctest --verbose'

.PHONY: fmt
fmt:
	find . -name "*.c" -o -name "*.h" | xargs clang-format -i

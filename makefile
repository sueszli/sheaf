DOCKER_RUN = docker run --rm -v $(PWD):/workspace main sh -c

.PHONY: run
run:
	scons -j$$(nproc)
	./binary
	scons -c

# .PHONY: memcheck
# memcheck:
# 	$(DOCKER_RUN) 'rm -rf build && mkdir -p build && cd build && cmake -DDISABLE_ASAN=ON .. && cmake --build . -j$$(nproc)'
# 	$(DOCKER_RUN) 'cd build && valgrind --leak-check=full ./binary'

# .PHONY: test
# test:
# 	$(DOCKER_RUN) 'rm -rf build && mkdir -p build && cd build && cmake -DBUILD_TESTS=ON .. && cmake --build . -j$$(nproc)'
# 	$(DOCKER_RUN) 'cd build && ctest --verbose'

.PHONY: fmt
fmt:
	find . -name "*.c" -o -name "*.h" | xargs clang-format -i

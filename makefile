DOCKER_RUN = docker run --rm -v $(PWD):/workspace main sh -c

.PHONY: run
run:
	scons -j$$(nproc) run
	@scons -c -s

.PHONY: test
test:
	scons -j$$(nproc) test
	@scons -c -s

.PHONY: memcheck
memcheck:
	scons -j$$(nproc) VALGRIND=1 valgrind
	@scons -c -s

.PHONY: fmt
fmt:
	find . -name "*.c" -o -name "*.h" | xargs clang-format -i

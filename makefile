DOCKER_RUN = docker run --rm -v $(PWD):/workspace sheaf sh -c

.PHONY: build-image # build the docker image
build-image:
	docker build -t sheaf .

.PHONY: run # run the main program
run: build-image
	$(DOCKER_RUN) 'scons run && scons --clean -s'

.PHONY: test # run all tests
test: build-image
	$(DOCKER_RUN) 'scons test && scons --clean -s'

.PHONY: valgrind # run the main program under valgrind
valgrind: build-image
	$(DOCKER_RUN) 'scons valgrind && scons --clean -s'

.PHONY: fmt # format all source files
fmt: build-image
	$(DOCKER_RUN) 'find . -name "*.c" -o -name "*.h" | xargs clang-format -i'

.PHONY: clean # remove all build artifacts
clean:
	docker rmi main 2>/dev/null || true

.PHONY: help # generate help message
help:
	@echo "Usage: make [target]\n"
	@grep '^.PHONY: .* #' makefile | sed 's/\.PHONY: \(.*\) # \(.*\)/\1	\2/' | expand -t20

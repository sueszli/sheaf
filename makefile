DOCKER_RUN = docker run --rm -v $(PWD):/workspace sheaf sh -c

.PHONY: build-image
build-image:
	docker build -t sheaf .

.PHONY: run
run:
	$(DOCKER_RUN) 'scons run && scons --clean -s'

.PHONY: test
test:
	$(DOCKER_RUN) 'scons test && scons --clean -s'

.PHONY: valgrind
valgrind:
	$(DOCKER_RUN) 'scons valgrind && scons --clean -s'

.PHONY: fmt
fmt:
	$(DOCKER_RUN) 'find . -name "*.c" -o -name "*.h" | xargs clang-format -i'

.PHONY: clean
clean:
	docker rmi main 2>/dev/null || true

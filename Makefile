build: clean
	mkdir -p build &&
	cd build &&
	cmake ../player
.PHONY: build

rebuild:
	mkdir -p build &&
	cd build &&
	cmake ../player
.PHONY: rebuild
	cd build &&
	cmake ../player
.PHONY: rebuild

clean:
	rm -rf build
.PHONY: clean
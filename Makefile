.PHONY: build rebuild clean

build: clean rebuild

rebuild:
	mkdir -p build
	cd build && cmake ../player

clean:
	rm -rf build

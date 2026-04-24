.PHONY: build rebuild clean install-build-dependencies package-apt-amd64 package-apt-arm64 package-apt-armhf

DIST_DIR ?= dist

run: rebuild
	./build/bin/xibo-player

run-options: rebuild
	./build/bin/xibo-options

build: clean rebuild

rebuild:
	rm -rf build
	mkdir -p build
	cd build && cmake ../player && make -j$(nproc)

clean:
	rm -rf build

install-build-dependencies:
	./scripts/install-build-deps.sh

package-apt-amd64:
	mkdir -p $(DIST_DIR)
	dpkg-buildpackage -us -uc -b -a amd64
	mv -f ../xibo-player_*_amd64.deb ../xibo-player_*_amd64.buildinfo ../xibo-player_*_amd64.changes ../xibo-player-dbgsym_*_amd64.ddeb $(DIST_DIR)/ 2>/dev/null || true

package-apt-arm64:
	mkdir -p $(DIST_DIR)
	if [ "$(shell dpkg --print-architecture)" = "arm64" ]; then \
		dpkg-buildpackage -us -uc -b -a arm64; \
	else \
		CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++ dpkg-buildpackage -us -uc -b -a arm64; \
	fi
	mv -f ../xibo-player_*_arm64.deb ../xibo-player_*_arm64.buildinfo ../xibo-player_*_arm64.changes ../xibo-player-dbgsym_*_arm64.ddeb $(DIST_DIR)/ 2>/dev/null || true

package-apt-armhf:
	mkdir -p $(DIST_DIR)
	CC=arm-linux-gnueabihf-gcc CXX=arm-linux-gnueabihf-g++ dpkg-buildpackage -us -uc -b -a armhf
	mv -f ../xibo-player_*_armhf.deb ../xibo-player_*_armhf.buildinfo ../xibo-player_*_armhf.changes ../xibo-player-dbgsym_*_armhf.ddeb $(DIST_DIR)/ 2>/dev/null || true

.PHONY: build rebuild clean install-build-deps

build: clean rebuild

rebuild:
	mkdir -p build
	cd build && cmake ../player && make -j$(nproc)

clean:
	rm -rf build

install-build-deps:
	sudo apt install \
		build-essential \
		cmake \
		libgtest-dev \
		libgmock-dev \
		libspdlog-dev \
		libssl-dev \
		libzmq3-dev \
		libsqlitecpp-dev \
		libhowardhinnant-date-dev \
		libcrypto++-dev \
		libgstreamer1.0-dev \
		libgstreamer-plugins-base1.0-dev \
		libboost-dev \
		libboost-system-dev \
		libboost-thread-dev \
		libboost-filesystem-dev \
		libboost-date-time-dev \
		libboost-program-options-dev \
		libgtkmm-4.0-dev \
		libwebkit2gtk-4.1-dev
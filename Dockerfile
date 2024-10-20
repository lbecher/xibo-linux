FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-ubuntu-20.04

RUN apt install -y \
  pkgconf \
  libgtkmm-3.0-dev \
  libsqlite3-dev \
  libwebkit2gtk-4.0-dev \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev \
  libgstreamer-plugins-base1.0-0 \
  libspdlog-dev \
  libssl-dev \
  libzmq3-dev \
  libcrypto++-dev \
  libboost1.71-all-dev \
  libgtest-dev \
  libgmock-dev

ENV DEBIAN_FRONTEND=noninteractive

RUN git clone https://github.com/fnc12/sqlite_orm.git sqlite_orm \
  && cd sqlite_orm \
  && cmake -B build \
  && cmake --build build --target install

RUN curl -o date-tz.tar.gz -SL https://github.com/HowardHinnant/date/archive/v3.0.0.tar.gz \
  && tar -zxvf date-tz.tar.gz \
  && cd date-3.0.0 \
  && cmake . -DBUILD_TZ_LIB=ON -DBUILD_SHARED_LIBS=ON -DUSE_SYSTEM_TZ_DB=ON \
  && make -j4 \
  && make install

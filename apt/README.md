# Xibo Player - Pacote Debian (.deb)

Este diretório contém os scripts e arquivos necessários para criar um pacote Debian (.deb) do Xibo Player.

## Estrutura

```
apt/
├── DEBIAN/
│   ├── control              # Metadados do pacote
│   ├── postinst            # Script pós-instalação
│   └── postrm              # Script pós-remoção
├── usr/
│   ├── bin/                # Binários (serão copiados durante o build)
│   └── share/
│       ├── applications/   # Arquivos .desktop
│       ├── icons/          # Ícones (serão copiados durante o build)
│       └── xibo-player/    # Recursos (serão copiados durante o build)
├── build-deb.sh           # Script principal de build
├── install-dependencies.sh # Script para instalar dependências
└── README.md              # Este arquivo
```

## Como usar

### 1. Instalar dependências (opcional)

```bash
cd apt/
./install-dependencies.sh
```

### 2. Construir o pacote .deb

```bash
cd apt/
./build-deb.sh
```

This script will:
- Detect the host architecture automatically
- Install build dependencies
- Build third-party libraries (date-tz, sqlite-orm)
- Compile Xibo Player for the current architecture
- Update the control file with the correct architecture
- Create the .deb package

### 3. Install the package

The package name will include the detected architecture:

```bash
sudo dpkg -i ../xibo-player_1.8-R7_<architecture>.deb
sudo apt-get install -f  # If there are unsatisfied dependencies
```

Where `<architecture>` can be: amd64, arm64, armhf, i386, riscv64, etc.

### 4. Executar

Após a instalação, você pode executar:

- **Xibo Player**: `xibo-player` ou pelo menu de aplicações
- **Xibo Options**: `xibo-options` ou pelo menu de aplicações
- **Xibo Watchdog**: `xibo-watchdog` (normalmente usado internamente)

## Dependências

### Dependências de Runtime (instaladas automaticamente)
- libcrypto++6
- libboost-date-time1.71.0 (ou 1.74.0)
- libboost-filesystem1.71.0 (ou 1.74.0)
- libboost-program-options1.71.0 (ou 1.74.0)
- libboost-thread1.71.0 (ou 1.74.0)
- libglu1-mesa
- freeglut3
- libzmq5
- libgtkmm-3.0-1v5
- libcanberra-gtk3-module
- libwebkit2gtk-4.0-37
- libgpm2
- libslang2
- gstreamer1.0-plugins-good
- gstreamer1.0-plugins-base
- gstreamer1.0-gl
- gstreamer1.0-libav
- gstreamer1.0-gtk3
- libspdlog1
- libsqlite3-0

### Dependências de Build (instaladas durante o build)
- cmake
- g++
- pkg-config
- libcrypto++-dev
- libboost-all-dev
- libgtkmm-3.0-dev
- libwebkit2gtk-4.0-dev
- libglibmm-2.4-dev
- libzmq3-dev
- libspdlog-dev
- libssl-dev
- libsqlite3-dev
- libcurl4-gnutls-dev

## Desinstalar

```bash
sudo apt remove xibo-player
```

## Notas

- O pacote é construído para a arquitetura do host atual (detectada automaticamente)
- Arquiteturas suportadas: amd64, arm64, armhf, i386, riscv64, e outras
- O processo de build baixa e compila algumas bibliotecas de terceiros localmente
- O pacote instala os binários em `/usr/bin/` e os recursos em `/usr/share/xibo-player/`
- Arquivos de configuração ficam no diretório home do usuário
- Caminhos do GStreamer são configurados automaticamente baseados na arquitetura alvo

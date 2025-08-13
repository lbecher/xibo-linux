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

Este script irá:
- Instalar dependências de build necessárias
- Construir as bibliotecas de terceiros (date-tz, sqlite-orm)
- Compilar o Xibo Player
- Criar o pacote .deb

### 3. Instalar o pacote

```bash
sudo dpkg -i ../xibo-player_1.8-R7_amd64.deb
sudo apt-get install -f  # Se houver dependências não satisfeitas
```

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

- O pacote criado é específico para a arquitetura amd64
- O processo de build baixa e compila algumas bibliotecas de terceiros localmente
- O pacote instala os binários em `/usr/bin/` e os recursos em `/usr/share/xibo-player/`
- Arquivos de configuração ficam no diretório home do usuário

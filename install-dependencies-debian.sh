#!/bin/bash

# Script para instalar dependências do Xibo Linux Player no Debian/Ubuntu
# Autor: Assistente IA
# Data: $(date +%Y-%m-%d)

set -e  # Para a execução se houver erro

echo "=== Instalador de Dependências do Xibo Linux Player ==="
echo "Este script instalará todas as dependências necessárias para compilar o Xibo Linux Player no Debian/Ubuntu"
echo

# Verificar se é Debian/Ubuntu
if ! command -v apt &> /dev/null; then
    echo "❌ Erro: Este script é apenas para sistemas Debian/Ubuntu com APT"
    exit 1
fi

# Verificar se está executando como root ou com sudo
if [[ $EUID -eq 0 ]]; then
    APT_CMD="apt"
else
    if ! command -v sudo &> /dev/null; then
        echo "❌ Erro: sudo não encontrado. Execute como root ou instale sudo"
        exit 1
    fi
    APT_CMD="sudo apt"
fi

echo "📦 Atualizando lista de pacotes..."
$APT_CMD update

echo
echo "🔧 Instalando ferramentas de build essenciais..."
$APT_CMD install -y \
    build-essential \
    cmake \
    git \
    pkg-config

echo
echo "📚 Instalando bibliotecas de desenvolvimento..."

# Core libraries
echo "  - Bibliotecas principais..."
$APT_CMD install -y \
    libsqlitecpp-dev \
    libsqlite3-dev \
    libcrypto++-dev \
    libzmq3-dev \
    libspdlog-dev \
    libfmt-dev

# Boost libraries
echo "  - Boost libraries..."
$APT_CMD install -y \
    libboost-all-dev

# GTK and GUI libraries
echo "  - Bibliotecas GTK e GUI..."
$APT_CMD install -y \
    libgtkmm-3.0-dev \
    libglibmm-2.4-dev \
    libwebkit2gtk-4.1-dev

# GStreamer libraries
echo "  - GStreamer libraries..."
$APT_CMD install -y \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools

# Date/Time libraries
echo "  - Bibliotecas de data/tempo..."
$APT_CMD install -y \
    libhowardhinnant-date-dev

# Testing libraries
echo "  - Bibliotecas de teste..."
$APT_CMD install -y \
    libgtest-dev \
    libgmock-dev

# OpenSSL and networking
echo "  - Bibliotecas de rede e criptografia..."
$APT_CMD install -y \
    libssl-dev \
    libcurl4-openssl-dev

# Additional system libraries
echo "  - Bibliotecas do sistema adicionais..."
$APT_CMD install -y \
    libx11-dev \
    libxss-dev \
    libdbus-1-dev \
    libxml2-dev

echo
echo "✅ Todas as dependências foram instaladas com sucesso!"
echo
echo "🚀 Agora você pode compilar o projeto:"
echo "   cd /caminho/para/xibo-linux"
echo "   cmake ./player"
echo "   make"
echo
echo "💡 Ou use a task configurada no VS Code: 'Build'"
echo

# Verificar se algum pacote importante falhou
echo "🔍 Verificando instalação das dependências principais..."

REQUIRED_PACKAGES=(
    "libsqlitecpp-dev"
    "libboost-dev"
    "libgtkmm-3.0-dev"
    "libgstreamer1.0-dev"
    "libwebkit2gtk-4.1-dev"
    "cmake"
)

MISSING_PACKAGES=()

for package in "${REQUIRED_PACKAGES[@]}"; do
    if ! dpkg -l | grep -q "^ii  $package "; then
        MISSING_PACKAGES+=("$package")
    fi
done

if [ ${#MISSING_PACKAGES[@]} -eq 0 ]; then
    echo "✅ Todas as dependências principais estão instaladas!"
else
    echo "⚠️  Atenção: Os seguintes pacotes podem não ter sido instalados corretamente:"
    for package in "${MISSING_PACKAGES[@]}"; do
        echo "   - $package"
    done
    echo
    echo "Tente instalar manualmente:"
    echo "   $APT_CMD install ${MISSING_PACKAGES[*]}"
fi

echo
echo "🎯 Instalação concluída!"

# Chess Game 

*[Read in Portuguese / Leia em Português](#-versão-em-português)*

---

# English Version

A Chess game developed in C++17 using the **SFML 3.0** library.
The project uses **CMake** as its build system, which means SFML is downloaded and compiled automatically and transparently.

---

## Prerequisites

To compile and run the project, you need to have the following installed on your computer:

1. **C++ Compiler** with C++17 support (e.g., GCC/G++, Clang, or MSVC/Visual Studio).
2. **CMake** (version 3.28 or higher).
3. **Git** (required for CMake to download SFML automatically).

---

## How to Compile and Run (Linux, Windows, and macOS)

The main build commands are **exactly the same** on any operating system.

Open your terminal (or Command Prompt/PowerShell) in the project's root folder and follow the two steps below:

### 1. Configure the project and download dependencies
This command prepares the build files and automatically downloads SFML 3.0 from GitHub. You only need to run this command the first time (or if you modify the `CMakeLists.txt` file).

```bash
cmake -B build
```

### 2. Compile the Game
This command transforms the C++ code into the game executable. Whenever you modify a `.cpp` code file, run this command again.

```bash
cmake --build build
```

---

## Running the Game

The generated executable will be located inside the `build/bin` folder. The command to open the game depends on your system:

### Linux / macOS:
```bash
./build/bin/main
```

### Windows (Command Prompt or PowerShell):
```cmd
.\build\bin\main.exe
```

---

## Specific Notes for Linux

If you use Linux (Ubuntu, Debian, Mint, etc.), SFML will need to communicate with your window system. You will need to install the development dependencies before compiling the game for the first time:

```bash
sudo apt-get install -y cmake git g++ \
    libxrandr-dev libxcursor-dev libxi-dev libudev-dev \
    libgl1-mesa-dev libflac-dev libogg-dev libvorbis-dev \
    libopenal-dev libfreetype-dev
```

<br><br><br>

---
---

# Versão em Português

Um jogo de Xadrez desenvolvido em C++17 utilizando a biblioteca **SFML 3.0**. 
O projeto utiliza **CMake** como sistema de build, o que significa que a SFML é baixada e compilada automaticamente de forma transparente.

---

## Pré-requisitos

Para compilar e rodar o projeto, você precisa ter instalados no seu computador:

1. **Compilador C++** com suporte a C++17 (ex: GCC/G++, Clang ou MSVC/Visual Studio).
2. **CMake** (versão 3.28 ou superior).
3. **Git** (necessário para o CMake baixar a SFML automaticamente).

---

## Como Compilar e Rodar (Linux, Windows e macOS)

Os comandos principais de compilação são **exatamente os mesmos** em qualquer sistema operacional.

Abra o terminal (ou Prompt de Comando/PowerShell) na pasta raiz do projeto e siga os dois passos abaixo:

### 1. Configurar o projeto e baixar dependências
Este comando prepara os arquivos de build e baixa a SFML 3.0 automaticamente do GitHub. Você só precisa rodar este comando na primeira vez (ou se modificar o arquivo `CMakeLists.txt`).

```bash
cmake -B build
```

### 2. Compilar o Jogo
Este comando transforma o código C++ no executável do jogo. Sempre que você alterar um arquivo de código `.cpp`, rode este comando novamente.

```bash
cmake --build build
```

---

## Executando o Jogo

O executável gerado ficará dentro da pasta `build/bin`. O comando para abrir o jogo depende do seu sistema:

### Linux / macOS:
```bash
./build/bin/main
```

### Windows (Prompt de Comando ou PowerShell):
```cmd
.\build\bin\main.exe
```

---

## Avisos Específicos para Linux

Se você usa Linux (Ubuntu, Debian, Mint, etc), a SFML precisará conversar com o seu sistema de janelas. Você precisará instalar as dependências de desenvolvimento antes de compilar o jogo pela primeira vez:

```bash
sudo apt-get install -y cmake git g++ \
    libxrandr-dev libxcursor-dev libxi-dev libudev-dev \
    libgl1-mesa-dev libflac-dev libogg-dev libvorbis-dev \
    libopenal-dev libfreetype-dev
```

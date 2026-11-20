# Interface gráfica - Xadrez-ES2

A interface usa C++17 e SFML 2.6.2. Ela inicia o motor UCI como subprocesso e troca
comandos por pipes.

## Windows com MinGW

Compile o motor primeiro:

```powershell
mingw32-make -C engine
```

Configure e compile a interface:

```powershell
cmake -S interface -B interface/build -G "MinGW Makefiles"
cmake --build interface/build --parallel
```

Execute:

```powershell
.\interface\build\xadrez.exe
```

A ponte procura automaticamente `engine/build/main.exe`. Os pipes usam a API Win32
e não dependem de `fork`, `/bin/cat` ou executáveis POSIX.

## Linux

```sh
make -C engine
make -C interface
./interface/build/xadrez
```

## Testes no Windows

```powershell
ctest --test-dir interface/build --output-on-failure
```

Os testes incluem smoke, unidade, integração com o motor real e recuperação.

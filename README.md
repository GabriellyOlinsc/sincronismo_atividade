# Atividade Concorrência
Este repositório contém duas implementações clássicas de problemas de concorrência utilizando C++17, threads, mutexes e condições:

- Jantar dos Filósofos
- Barbeiro Dorminhoco

Ambos foram desenvolvidos para fins educacionais.

## Pré-requisitos
- Compilador compatível com C++17 (g++, clang++)
- Suporte a POSIX threads (Linux/macOS) ou MinGW-w64 (Windows)

## Como rodar

Jantar dos Filósofos
```bash
g++ -std=c++17 -pthread jantar_filosofo.cpp -O2 -o jantar_filosofo
```

Barbeiro Dorminhoco
```bash
g++ -std=c++17 -pthread barbeiro.cpp -O2 -o barbeiro
```

## Execução
Algotimo Jantar Filosofos
```bash
./jantar_filosofo N duracao_seconds think_min_ms think_max_ms eat_min_ms eat_max_ms
```
Exemplo:
```bash
./jantar_filosofo 5 20 100 500 200 600
```

Algoritmo barbeiro dorminhoco
```bash
./barbeiro num_cadeiras chegada_min chegada_max atend_min atend_max duracao_seg
```
Exemplo:
```bash
./barbeiro 5 200 600 300 700 20
```




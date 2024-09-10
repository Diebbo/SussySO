# SussySO
##### Diego Barbieri, Alex Rossi, Giuseppe Iannone, Vittorio Massimiliano Incerti
## Installazione
- seguire gli step per compilare uriscv
- lanciare il comando seguente

## Test

```bash
make
uriscv-cli --config ./config_machine.json --gdb
```


## debug

```bash
make clean && make && uriscv-cli --config ./config_machine.json --gdb
```

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

## Submit
1. zip the source with tar
2. submit the tar file with rsync
3. check the hash

```bash
make clean
tar -czvf sussyso.tar.gz phas1/ phas2/ phas3/ Makefile header/ AUTHOR.md config_machine.json README.md Documentation.pdf
sha1sum sussyso.tar.gz # check the hash
rsync -avz sussyso.tar.gz nome.cognome@marullo.cs.unibo.it:/home/students/LABSO/2024/submit_phase3.final/lso24az1
ssh nome.cognome@marullo.cs.unibo.it:/home/students/LABSO/2024/submit_phase3.final/lso24az1
sha1sum sussyso.tar.gz # check the hash


```

## debug

```bash
make clean && make && uriscv-cli --config ./config_machine.json --gdb
```

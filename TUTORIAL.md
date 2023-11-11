# Tutorial: come far funzionare uRISCV

## Dependencies

### RISCV Toolchain

```bash
git clone https://github.com/riscv/riscv-gnu-toolchain
sudo apt-get install autoconf automake autotools-dev curl python3 python3-pip libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev
cd riscv-gnu-toolchain
./configure --prefix=/opt/riscv --with-arch=rv32gc --with-abi=ilp32d
sudo make linux
```

### Pacchetti Debian

```bash
sudo apt install git build-essential libc6 cmake libelf-dev libboost-dev libboost-program-options-dev libsigc++-2.0-dev gcc-riscv64-unknown-elf
```

### Compilare e installare l'emulatore
```bash 
mkdir -p build && cd build
cmake .. && make && sudo make install 
```

### Eseguire il progetto sull'emulatore
Posizionarsi nella cartella con l'eseguibile `kernel` ed eseguire il seguente comando:
```bash
uriscv-cli --config ./config_machine.json
```

#### Troubleshooting
- se i comandi della toolchain non si trovano dopo l'installazione, aggiurnare la variabile d'ambiente `PATH` aggiungendo `/opt/riscv/bin`
- se eseguire l'emulatore da errore il seguente errore
```bash
terminate called after throwing an instance of 'FileError'
  what():  Error accessing `/usr/share/uriscv/exec.rom.uriscv'
[1]    6801 IOT instruction  uriscv-cli --config ./config_machine.json
```
controllare i path di bootstrap-rom e execution-rom (i path potrebbero essere `/usr/share/uriscv/...` oppure `/usr/local/share/uriscv/...`)

### Tutorial per debbuggare su vscode:
1. Copiare il file `launch.json` nella cartella `.vscode` (controllare che i percorsi siano giusti)
2. Assicurarsi di compilare il progetto con la flag `-ggdb`
3. Eseguire il tool con il comando `uriscv-cli --gdb --config ./config_machine.json` nella cartella con l'eseguibile `kernel`
4. Avviare il debugger su vscode
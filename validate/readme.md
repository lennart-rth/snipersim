`gcc -static -g -O0 minimal.c -o minimal`
`gcc -c -g -O0 minimal.c -o minimal.o` and `objdump -d -M intel -S minimal.o`


'./run-sniper -c haswell -n 1 --roi -- ./validate/minimal'



# Shell
A basic shell written in C that can execute commands and piped commands

# Instructions
```
make
gcc -g -std=gnu11 -Wpedantic -Wall   -o shell shell.o
./shell
```

# What I learned
1. Parsing inputs in C
2. Creating processes
3. Piping
4. fork(), dup(), pipe(), fgets(), strtok() syscalls
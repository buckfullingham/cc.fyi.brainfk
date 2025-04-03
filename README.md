# cc.fyi.brainfk

![build status](https://github.com/buckfullingham/cc.fyi.brainfk/actions/workflows/build.yml/badge.svg)

# Build Your Own Pong

This is a solution to the problem posed at https://codingchallenges.fyi/challenges/challenge-brainfuck.

## Features

1. There is a basic repl, "ccbf," which you can use to execute brainfuck scripts.
2. You can specify whether you want to use the "handrolled" or "llvm" virtual machines.
3. handrolled compiles to and then executes bytecode.
4. llvm JIT compiles to and then executes native machine code.

### Usage

Execute a script using the default (handrolled) machine:

```shell
$ ccbf mandalbrot.bf
```

Execute a script using the llvm machine:

```shell
$ ccbf -m llvm mandalbrot.bf
```

Using ccbf as a repl (note an empty line signifies end of the script):

```shell
$ ccbf
ccbf> ++++++++++[>+>+++>+++++++>++++++++++<<<<-]>>>++.>+.+++++++..+++.<<+++.
ccbf> 
Hello!
ccbf> 
```


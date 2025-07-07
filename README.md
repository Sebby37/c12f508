# c12f508 - A PIC12F508 emulator written in C
It's exactly what it sounds like! Just a little project for me and no one else really I guess. It's my first time using C for a proper project though, 
so I'm learning as I go when it comes to best practices, so don't expect the super best practices in syntax and such. It's currently incomplete, 
see the todo just below for what (I think) needs to be done before it's fully functional. Also it's C99 compatible, because why not?

## Todo
- [X] Setup the project (idk I'm writing this with all of the crossed out stuff done at the moment)
- [X] Implement every instruction (except SLEEP and CLRWDT)
- [X] Cycle counting
- [X] HEX file reading (see tests/test_divide.c)
- [X] Watchdog timer functionality (+ CLRWDT)
- [X] timer0 functionality
- [~] Proper reset function(s) + functionality
- [ ] Proper pin IO functions
- [~] Configuration word being acknowledged (kinda requires all of the above)
- [ ] SLEEP instruction
- [ ] Make it thread-safe!
- [~] Better makefile or even CMake
- [ ] Make it cycle accurate (as in the whole 2-stage pipeline and 4-cycle fetch/execution)

## Future plans that maybe might just potentially happen
- Full main program that can debug and such with a CLI interface
- Some way of specifying pin configurations through JSON or something similar
- A GUI of some kind, similar to the Nand2Tetris CPU emulator

## Running
Very simple, clone the project and run `make`, then you'll get the currently very unfinished main program that does nothing as of yet. 
The tests/ directory will contain my tests though which should have actual functionality! Run `make tests` for all the tests (1) you could possibly ever want!

## Motivation
I just like the 12f508, it's very simple to learn in terms of architecture and assembly, there's not that many instructions or registers either. 
Also I have some personal projects built using it that I'd like to one day try build a GUI for in some capacity. So far its been a fun project, writing an emulator
and running your code on it! In the future maybe I'll do a 6502 emulator, and probably in C too!

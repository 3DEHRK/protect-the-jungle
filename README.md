# Protect The Jungle: monkeys fight back!

#### Required to build: gcc compiler -> https://filestash.gibb.ch/files/IET-Share/sh-classes/inf-306-21l/ or https://mega.nz/file/G6xCXarZ#aXHECbsb8Ddlxk9fQARzLK8MmrpxgCmmBqKFkZyQdec -> Add Path Variable C:\mingw32\bin
#### SFML Docs: https://www.sfml-dev.org/tutorials/2.6

<br/>

## 🪁✨ Getting started 🚀🎯
1. Check out the C++ Hints below 💪
2. Take a look at the Entity class (line ~200) 🤔
3. See what functions the Game class offers by reading it's comments (line ~250) ☺️
4. Get a glimpse of how exisitng game entities work (line ~650) 🤓👆
5. Commit something & have fun! 💜

<br/>

## C++ Hints
### std::vector: Array like container with extra functionality
### Entity* entity = new Entity(); A pointer variable that holds the memory adress of an object (created on the heap)
### to access a member through a pointer you use ptr->do() instead of obj.do()
### Zombie& zombie; & means take the object by reference instead of copying it

<br/>

![Alt Text](/uml/classes.svg)

<br/>

## Ideas

### Zombies
1. Lane switching
2. Piggyback
3. slow but high hp tank göy so you have to take action
4. ... 🦋

### Plants
1. Minion spawning
2. Split damage & knockback, short & longrange ...
3. ... 🧙‍♂️
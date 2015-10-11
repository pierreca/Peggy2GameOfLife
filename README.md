This small project is a naive implementation of [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) on a [Peggy 2](http://shop.evilmadscientist.com/productsmenu/75) board.

![Image of the Peggy 2 board](http://raw.github.com/pierreca/Peggy2GameOfLife/master/peggy2frame.jpg)

# How it works
The algorithm starts as soon as the Peggy 2 board is powered. It will randomly initialize the first frame, then run Conway's rules on each cells.
There is a simplistic loop detection that will stop the iterations if it detects that the life-form is stuck in a basic loop (we'll get back to that).
Once it stops, it will display how many steps were taken to get there, as well as the maximum number of steps ever reached since the Peggy board has been powered on.

# Compiling the code
You will need the Peggy2Serial library in your Arduino librairies folder. To install this library see the [Peggy 2 product page](http://shop.evilmadscientist.com/productsmenu/75) and [associated blog post](http://www.evilmadscientist.com/2008/peggy-version-2-0/)

# Implementation details and conventions
## Dependency on the Peggy2Serial library
This project was written specifically to run on a Peggy2 board. With that in mind, we're using the basic framebuffer of the Peggy2 library as the grid in which cells live, which creates a dependency on the Peggy2Serial library. We could abstract that with the right pattern in the future, being careful not to introduce a performance hit.

## The infinite canvas
Conway's rules suppose that they run on an infinite canvas. To simulate this on a 25x25 matrix, we "loop" around the edges:
the cell at (0,0) is considered neighbors with (0,1), (1,1), (1,0) as well as (24,0), (24,1), (0,24), (1,24), and (24,24).

This should be made an optional parameter as it helps creating undetectable loops. 

## How to detect when to stop
The rules engine keeps in memory a specific number of old frames (it's a parameter of the engine). At each iteration, it compares a frame with the old frames in memory: if it finds 2 frames are identical, which means we're stuck in a loop, it stops. 

I've found that 4 is a good number to detect the most common forms of oscillators without slowing down the display of each step. It will not detect spaceships and oscillators with a period larger than what is specified when initializing the rules engine though.

There are other things we could do:
* detect an unusally large number of steps (arbitrary)
* actively scan for gliders (computationally very expensive)
* store more frames (25 would be enough because of the infinite canvas logic, but expensive memory-wise)
* Renounce the infinite canvas logic (and consider all cells outside of the matrix are dead). the glider would eventually exit.

## Stepping speed
The time to progress from the current step to the next is defined in the code and can be accelerated or slowed down to accomodate for modifications of the above conventions, for example, if we want to compare more frames.
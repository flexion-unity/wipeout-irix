# WipEout-rewrite for IRIX 6.5.x

To compile on IRIX, use the Makefile instead of cmake.

Compiles without errors with SGUG-RSE gcc and toolchain.
Needs SDL2 with GL support. 

Note: the SDL2 package in the SGUG-RSE repo currently does not support GL.

To run the game, place the assets from your licensed WipEout CD in the folder ./wipeout/ and then run ./wipegame from the root of the repository.


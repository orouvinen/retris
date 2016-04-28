Retris Tetris clone I wrote a long time ago.

Here's a screenshot in all its 320x200 pixels glory (running in DOSBox)

![Retris](https://github.com/orouvinen/retris/screenshots/retris-screenshot.png?raw=true)


If you truly want to build it, you will need Borlanc C++ 3.1 compiler, or at least
that's what I used back then to compile it.
Running COMPILE.BAT should do the trick.

Included is a VGA mode 13h graphics library I wrote in assembly, because it was
good fun back then. The assembled object file that can be linked with rest of the program
is included (and will be linked in by COMPILE.BAT) so that you don't have to
dig out your assembler as well in order to build this :). You would need Microsoft Assembler for that if I remember correctly.

And as a last word, the code was written, as I said, long time ago and there are things
I would probably do differently if I was to write this now.



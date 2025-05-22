# Mini vMac

Mini vMac is a miniature Macintosh 68K emulator.  
The original version of this software was written by Paul C. Pratt.

## Building Mini vMac

Use one of the build scripts in the top level of this repository as a starting point, editing the arguments to the setup tool as needed to customize the model and features of the Macintosh being emulated, and to specify the platform on which it is intended to run.

By default, Mini vMac emulates a Macintosh Plus with a 512x342 monochrome display. Other 68K-based Mac models can be emulated by specifying a different model with the `-m` option. See the [Building Mini vMac page](https://www.gryphel.com/c/minivmac/build.html) for details.

### Building the Kanji (Japanese Mac Plus) variant
The [recently discovered](https://www.journaldulapin.com/2025/05/17/the-lost-japanese-rom-of-the-macintosh-plus-which-isnt-lost-anymore/) Japanese Mac Plus 256K ROM, which contains built-in KanjiTalk fonts for better performance, can now be used with Mini vMac. To emulate a Kanji model which can use this ROM, you can specify the new `-m Kanji` option in the setup tool. For example, this builds the Kanji variant for Apple Silicon, also enabling LocalTalk-over-UDP networking:

	./setuptool -n "minivmac-37.03-kanji" \  
	  -m Kanji -t mcar -lt -lto udp -sgn 0 > setup.sh

### Further reference:
[Main development website](https://www.gryphel.com/)

[Mirror of main development website as of 05/25/22](https://minivmac.github.io/gryphel-mirror/index.html)

[State of affairs](https://www.emaculation.com/forum/viewtopic.php?t=11570)


STK-libmapper demo synth
========================

This is a very simple synth demonstrating how to use the `instances` API from [libmapper][1] to control a synth built with the [Synthesis ToolKit in C++ (STK)][2].

This software is licensed with the GPLv3; see the attached file
COPYING for details, which should be included in this download.

To build, you will first need to install [libmapper][1] and [liblo][3] from their respective repositories. An installation of liblo <= v0.26 is not sufficient, as libmapper depends on new functionality in the library.

Joseph Malloch 2013
[Input Devices and Music Interaction Laboratory][4], McGill University.

## Changelog: 20170901

* Updated to libmapper v1.0

[1]: http://github.com/libmapper/libmapper
[2]: https://ccrma.stanford.edu/software/stk/
[3]: http://github.com/radarsat1/liblo
[4]: http://idmil.org/software/libmapper
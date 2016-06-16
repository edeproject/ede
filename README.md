# EDE

EDE is small and fast desktop environment that uses
[FLTK toolkit](http://www.fltk.org). For more details and philosophy
behind it, check
[about EDE on our wiki](http://equinox-project.org/wiki/AboutEde).

## Build requirements

EDE is using FLTK toolkit from http://www.fltk.org. At the time of
this writing, the latest stable tree is 1.3.x.

Since FLTK lacks many things needed for developing a full *nix desktop
environment, we have developed a small add-on library called
`edelib`. This library is also required for compiling EDE and is
released together with EDE.

We *strongly* recommend that you use matching versions of EDE and
edelib (e.g. released at the same time) or you make repository
checkout at the same time due their frequent changes.

Also you will need a `jam` tool. Jam is a *make* replacement and you
can find it on our repository.

## Downloading the code

The best way to download latest code is checking it out from our
repository. These modules you should checkout (with their paths):

- *jam* - `git clone https://github.com/edeproject/jam.git`
- *edelib* - `git clone https://github.com/edeproject/edelib.git`
- *ede* - `git clone https://github.com/edeproject/ede.git`

If you already have Jam installed, there is no need to download it.

## Compiling and installing

In order to build and install EDE do the following steps:

1. compile and install `jam` first; going to jam source directory and running `make` will do
   the job; after that you should get `bin.YOUR_PLATFORM` directory (on Linux it will be
   bin.linux) and copy `jam` executable in your $PATH, e.g. */usr/local/bin*

2. compile and install `edelib`; please read README file in edelib directory

3. go in ede directory and run `./autogen.sh`

4. after that, goes `./configure --enable-debug`

5. jam

6. jam install

Please note how this document is quick and short tutorial about EDE installation. For more details,
please check [Installation Howto](http://equinox-project.org/wiki/InstallationHowTo) on our wiki.

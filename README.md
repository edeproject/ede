# EDE

EDE, the `Equinox Desktop Environment`, is a small and fast desktop
environment that uses the [FLTK toolkit](http://www.fltk.org).
For more details and the philosophy behind it, see
[about EDE on our wiki](http://equinox-project.org/wiki/AboutEde).

## Build requirements

EDE requires FLTK; at the time of this writing, the latest stable
branch is 1.3.x.

Since FLTK lacks many things needed for developing a full *nix desktop
environment, we have developed a small add-on library called
`edelib`. This library is needed both for building and running the
desktop. Edelib is developed and released together with EDE.

It is *strongly* recommended to use matching versions of EDE and
edelib (i.e. versions released at the same time) or to checkout
both from the repository at the same time to make sure they work
together well.

Also you will need the `jam` tool. Jam is a *make* replacement and you
can find it on our repository.

## Downloading the code

The best way to get the latest code is checking it out from our
repository. These are the modules you should checkout (with their paths):

- *jam* - `git clone https://github.com/edeproject/jam.git`
- *edelib* - `git clone https://github.com/edeproject/edelib.git`
- *ede* - `git clone https://github.com/edeproject/ede.git`

If you already have Jam installed, there is of course no need to download
it again. Either vanilla Jam or FTJam can be used to build EDE. Boost Jam is
known to *not* work.

## Compiling and installing

In order to build and install EDE do the following steps:

1. compile and install `jam` first; going to jam source directory and running `make` will do
   the job; after that you should get `bin.YOUR_PLATFORM` directory (e.g. on Linux it will be
   bin.linux) and copy `jam` executable in your $PATH, e.g. */usr/local/bin*

2. compile and install `edelib`; please read README file in edelib directory

3. change into the ede directory and run `./autogen.sh`

4. after that, do `./configure --enable-debug`

5. jam

6. jam install

Please note that this document is only a quick and short tutorial on installing EDE. For more details
please see [Installation Howto](http://equinox-project.org/wiki/InstallationHowTo) on our wiki.

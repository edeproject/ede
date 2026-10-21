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

Also you will need the `jam` tool. Jam is a *make* replacement. On Debian
and Ubuntu install the `jam` package: that is Perforce Jam 2.5/2.6, which
is what `./configure` accepts. FTJam is fine too, where the package still
exists (it was dropped after Ubuntu 22.04). Boost Jam (`b2`) does not work.

The copy in `edeproject/jam` is optional and is not used by CI. It is a
Haiku-based fork whose version string is not numeric, so this tree's
configure check rejects it.

## Downloading the code

The best way to get the latest code is checking it out from our
repository. These are the modules you should checkout (with their paths):

- *jam* — not required as source. Use the system `jam` package (Perforce Jam or FTJam).
- *edelib* — submodule `external/edelib`, or `git clone https://github.com/edeproject/edelib.git`
- *ede* — this repository

If you already have Jam installed, there is of course no need to download
it again. Either vanilla Jam or FTJam can be used to build EDE. Boost Jam is
known to *not* work.

`edelib` is a git submodule at `external/edelib`, pinned to a commit of
`edeproject/edelib`. Clone this repository with
`git clone --recurse-submodules`. Jam is intentionally not a submodule:
use the system binary described above.

## Continuous integration

Linux CI is [`.github/workflows/linux.yml`](.github/workflows/linux.yml).
It runs on Ubuntu 24.04 (pinned, not `ubuntu-latest`), installs the distro
`jam` package, builds and tests the `edelib` submodule, then configures
and installs EDE with that same Jam. The manual is not built there.

## Compiling and installing

In order to build and install EDE do the following steps:

1. install `jam` from the system (see above). Building `edeproject/jam` is not required.

2. build and install `edelib` from `external/edelib` (or a matching checkout). See its README. Install it before configuring EDE, for example with `--prefix=/usr/local`.

3. change into the ede directory and run `./autogen.sh`

4. after that, do `./configure --enable-debug` (add `--with-edelib-path=DIR` if edelib is not in the default pkg-config path)

5. jam

6. jam install

Please note that this document is only a quick and short tutorial on installing EDE. For more details
please see [Installation Howto](http://equinox-project.org/wiki/InstallationHowTo) on our wiki.

crimild-demos
=============

A repository for crimild-based demos

Checkout
--------

  git clone <REPO_URL> --recursive

Project setup (Makefile)
------------------------

  cmake .

Building (Makefile)
-------------------

All: 

  make clean all test -j4

Project setup (Xcode)
---------------------

  cmake . -G Xcode

Project setup (Visual Studio)
-----------------------------

  cmake . -G "Visual Studio 2014"

Updating code
-------------

  git pull origin devel
  git submodule update --recursive
  cmake .
  make all -j4


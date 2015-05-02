This repository provides a modified version of the AVR simulator `simavr
<https://github.com/buserror/simavr>`_ mainly written by Michel Pollet and released under the GPL 3
license.

Main motivations was to help to understand how it works and then extend and use it.

This repository features:

* a CMake build (which is also able to compile firmwares)
* a reorganisation of the file hierarchy (e.g. broken parts was hidden in sub-directories)
* a reindentation of the code to get rid of tabs and to homogenise coding standards
* Doxygenation of the comments so as to generate an API documentation
* additional featured examples to show the simulator in action
* a Python binding using cffi
* data on AVR instructions and registers extracted semi-automatically from datasheets.
  
The original README was renamed README-simavr.md to avoid clash with this one.

TODO: wipe this repository and rebuild a clean one so as to facilitate cherry picking

* rename Makefile to Makefile.legacy so as to break accidental make execution within the sources
* reorganise the sources
* inject the CMake build: building is operational on unmodified sources
* apply cosmetic on sources: could introduce bugs ... 
* add stuffs

=============
 Motivations
=============

CMake
~~~~~

CMake is a powerful build system which improves dramatically the maintainability and portability of
the building process.

Code reindentation, coding homogenisation and reorganisation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Despite it is a bad idea to change the coding of a source code since it is subjective, will make
difficult the tracking of posterior modifications in the sources, and potentially introduce
bugs. The usage of tabs for indentation is evil, since the space's size is not universally
defined. Any serious editor must indent code using spaces only. Moreover it is always a good
practice to homogenise the coding, to define rules and enforce their usages.

TODO: provide the reindent recipe

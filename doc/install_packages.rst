.. highlight:: bash

.. _install_package:

Packages
========

OS X (brew)
-----------

Experimental packages of ``c2py`` and ``clair`` are::

  brew install parcollet/ccq/c2py  parcollet/ccq/clair

They are built from source from the GitHub parcollet/ccq repository.

.. note::

   To use the plugin as in the example below, 
   the DYLD_LIBRARY_PATH must be set e.g.::

     export DYLD_LIBRARY_PATH=/opt/homebrew/lib/:$DYLD_LIBRARY_PATH

   for clang to find its plugins in brew directory as in the documentation examples.
   Alternatively, you can provide the full path to the plugin.
   This setup is not necessary to use the CMake targets.
   


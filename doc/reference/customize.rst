.. _customize:

Customization
*************

``clair-c2py`` is an automatic tool. 
It wraps functions, classes and enums that it discovers
in the C++ source files, except those declared in system headers i.e. the standard library and more generally 
anything the compiler includes via ``-isystem`` options.

This default behavior of the bindings generation can be **customized** in two ways:

.. toctree::
   :maxdepth: 2

   customize/code_annotations
   customize/toml


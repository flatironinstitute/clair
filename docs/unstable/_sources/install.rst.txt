.. highlight:: bash

.. _install:

Installation
************

clair/c2py is made of two components:


* `c2py <https://github.com/flatironinstitute/c2py>`_
 
   Low level C++20 support library for the bindings.
   It depends only on the Python/Numpy C API and can be used with any C++20 compliant compilers (NB : currently tested on clang >=18 and gcc >=12). 
  
* `clair <https://github.com/flatironinstitute/clair>`_
  
   | A collection of Clang tools.
   | The `clair/c2py` clang tool generates the Python bindings from the C++ source code. 
 

 
.. toctree::
   :maxdepth: 2

   install_src

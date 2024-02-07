.. highlight:: bash

.. _install:

Installation
************

Clair/c2py is made of two components:


* **c2py**
 
  Low level C++20 support library for the bindings.
 
  It depends only on Python/Numpy C API and can be used with any C++20 compliant compilers 
  (NB : currently tested on clang >=16 and gcc >=12). 
  
* **clair** 
  
  A collection of Clang plugins. 
  
  Clair/c2py plugin generates the Python bindings automatically from the C++ source.

  Can only be used with LLVM/clang and with the *exact version of clang for which it has been compiled*. 
 

 
.. toctree::
   :maxdepth: 2

   install_packages
   install_src

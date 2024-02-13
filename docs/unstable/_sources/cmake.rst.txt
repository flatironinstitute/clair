.. _cmake:

CMake integration
-----------------

Let us consider a simple piece of C++ code and a `CMakeLists` file to compile it.

.. literalinclude:: examples/cmake1/my_module.cpp
   :language: cpp
   :linenos:

Compiling with clang and the clair plugin
.........................................

To use the clair plugin, we can use the following `CMakeLists.txt` file

.. literalinclude:: examples/cmake1/CMakeLists.txt
   :language: cmake
   :linenos:

This is a standard ``CMake`` configuration file.
`Python_add_library` declares a C++ Python extension using the standard CMake 
`FindPython <https://cmake.org/cmake/help/latest/module/FindPython.html>`_ package.
The additional steps required by ``clair`` are:


* [line 7-8] Require the cmake packages `c2py` and `clair`.
  
* [line 13] Compile against two targets:

  * ``c2py::c2py``: compile and link against c2py
  * ``clair::c2py_plugin``: add the `-fplugin` instruction to `clang`


A more general setup
....................

The previous example is fine to develop the project, but needs to be generalized
if we want to deploy the code on more platforms, without clang or clair, i.e. just using the bindings without generating them.
We assume that the bindings previously generated have been copied into the sources.
The `CMakeLists.txt` reads:
   

.. literalinclude:: examples/cmake2/CMakeLists.txt
   :language: cmake
   :linenos:

We introduce an option `GENERATE_PYTHON_BINDINGS` so that the code can not be used in two ways:

* If ``GENERATE_PYTHON_BINDINGS=ON``, it is the same as before.

* If ``GENERATE_PYTHON_BINDINGS=OFF``, it simply compiles the bindings `my_module.wrap.cxx` from the sources, without any plugin.
  This compiles with any C++20 compiler (tested actually on clang >=16, gcc >= 12), and requires only Python and `c2py`.

.. note::
   
   The CMake syntax *$<...>*  are `expression generators <https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html#conditional-expressions>`_, 
   and basically an `if` expression in CMake.

A variant avoiding the installation of c2py
...........................................

It is often convenient to avoid the need to install the `c2py` library completely. 
As `c2py` is a very small library, the cost of recompiling it in each project is negligeable, 
but it is often a good strategy to ensure it is compiled with the same compilers, standard libraries, etc.

In order to do this, we can simply modify the `CMakeLists.txt` file as 

.. literalinclude:: examples/cmake3/CMakeLists.txt
   :language: cmake
   :linenos:

In line 7-14, we use the `FetchContent <https://cmake.org/cmake/help/latest/module/FetchContent.html>`_ command of CMake 
to fetch the sources of c2py during the configuration.
The rest of the script is unchanged.



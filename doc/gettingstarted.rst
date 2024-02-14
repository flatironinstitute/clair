.. _gettingstarted:

Getting started
===============

Starting example
----------------

Let us begin with a simple example:

.. literalinclude:: examples/gs1.cpp
   :language: cpp

We just compile the C++ file with `clang` using the ``clair`` plugin as 

.. code-block:: bash
   
   # OS X
   $ clang++ -fplugin=clair_c2py.dylib  my_module.cpp -std=c++20 -shared -o my_module.so `c2py_flags`
   
   # Linux
   $ clang++ -fplugin=clair_c2py.so     my_module.cpp -std=c++20 -shared -o my_module.so `c2py_flags`

.. note::

   The compiler searches for the plugin in ``DYLD_LIBRARY_PATH`` (OS X) or ``LD_LIBRARY_PATH`` (linux).
 
That is it. The Python module is ready to be used:

.. code-block:: console

   >>> import my_module as M
   >>> M.add(1,2)
   3

The generation (and compilation) of the C++/Python binding code is **automatically done**
by the clang compiler. 

The C++ documentation is also automatically transformed into standard `numpydoc` format.

.. code-block:: console

   >>> help(M.add)
       add(...)
           Dispatched C++ function
           [1]  (x: int, y: int) -> int

              Some documentation

              Parameters
              ----------

              x:
                 First value
              y:
                 Second value

              Returns
              -------

              The result


What happened ?
................

In order to call C++ from Python, some **binding code** has to be generated.
It is a piece of C++ code which *adapts* the C++ functions and classes to the C API of Python.

The ``clair_c2py``  **compiler plugin** automatizes this task, by modifying the compilation process of LLVM/clang.
When compiling the C++ code `my_module.cpp` with it, the compiler

#. parses `my_module.cpp` as usual (the first step of a compilation: check syntax and grammar, build the Abstract Syntax Tree or AST),
#. generates the C++/Python bindings (by analyzing the AST) and writes them into `my_module.wrap.cxx` and
#. compiles the whole code (source + bindings) into a Python module `my_module.so`.

Reusing the bindings without clang
..................................

While the generation of the bindings requires LLVM/clang and the clair plugin, 
they can be reused with any `C++20` compiler. For example, with gcc:

.. code-block:: bash

   $ g++  my_module.wrap.cxx -std=c++20 -shared -o my_module.so `c2py_flags`


(the binding file includes the original source file `my_module.cpp`).

.. note::

   On some machines (OS X), beware that ``c2py`` must be compiled with gcc (in particular the same std C++ library).
   For any project beyond a simple demo, it is recommended to use :ref:`cmake`.

A typical workflow is therefore:

* As the developer of the code, use clang with the ``clair`` plugin. Bindings are automatically regenerated when the C++ API change.

* As a user of the code, just compile the bindings on any machine/compiler.

  
Section :ref:`cmake` shows how to use ``c2py`` and ``clair`` in a CMake project, 
including an example with a simple option to regenerate the bindings or not.

.. note::

  Even though the bindings are readable C++ code, they are designed to be automatically generated, not written by hand.
  The ``c2py`` API is therefore not part of the user documentation.



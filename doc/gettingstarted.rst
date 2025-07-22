.. _gettingstarted:

Getting started
===============

Starting example
----------------

Let us begin with a simple example:

.. literalinclude:: examples/gs1.cpp
   :language: cpp

We call the ``clair-c2py`` binding generator and the compiler:

.. code-block:: bash
   
     clair-c2py my_module.cpp -- -std=c++20 `c2py_flags -i`
     clang++ my_module.cpp -std=c++20 -shared -o my_module.so `c2py_flags`
   
That is it. The Python module is ready to be used:

.. code-block:: console

   >>> import my_module as M
   >>> M.add(1,2)
   3

* The first command ``clair-c2py``: 
  
  #. Generates the C++/Python bindings code into `my_module.wrap.cxx`.
  #. Adds an ``#include "my_module.wrap.cxx"`` at the end of the original source file `my_module.cpp`.

* The second command ``clang++`` compiles the C++ code (including the binding code) into a Python module `my_module.so`.

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


What happened?
...............

In order to call C++ from Python, some **binding code** has to be generated.
It is a piece of C++ code which *adapts* the C++ functions and classes to the C API of Python.

The ``clair_c2py`` tool automatizes this task, as it:

#. Parses `my_module.cpp` as usual (the first step of a compilation: check syntax and grammar, build the Abstract Syntax Tree or AST).
#. Generates the C++/Python bindings (by analyzing the AST) and writes them into `my_module.wrap.cxx`.

.. note::

   You can use any C++20 compiler to compile the bindings (clang, gcc, etc.), it is independent of the ``clair_c2py`` tool (and on the LLVM/clang version it is based on).

Section :ref:`cmake` shows how to use ``clair`` in a CMake project, 
including an example with a simple option to regenerate the bindings or not.

.. note::

  Even though the bindings are readable C++ code, they are designed to be automatically generated, not written by hand.
  The ``c2py`` API is therefore not part of the user documentation.

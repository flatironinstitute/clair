.. _gettingstarted:

Getting started
===============

Starting example
----------------

.. note::

   | This is an example of manually calling the tool. It requires installing ``c2py``.
   | Using ``clair-c2py`` in a ``CMake`` project is described in the section :ref:`cmake`.


Let us begin with a simple example:

.. literalinclude:: examples/gs1.cpp
   :language: cpp

We call the ``clair-c2py`` binding generator and the compiler.

.. code-block:: bash
   
     clair-c2py my_module.cpp -- -std=c++20 `c2py_flags -i`
     clang++ my_module.cpp -std=c++20 -shared -o my_module.so `c2py_flags`
   
That is it. The Python module is ready to be used:

.. code-block:: console

   >>> import my_module as M
   >>> M.add(1,2)
   3

Let us walk through the commands in details:

Python C++ Bindings Generation
..............................

The following command generates the Python bindings from a C++ module:

.. code-block:: bash
   
   clair-c2py my_module.cpp -- -std=c++20 `c2py_flags -i`

This performs the following steps:

1. Parses the C++ code and generates the C++/Python bindings into the file ``my_module.wrap.cxx``.

2. Appends the following line to the end of the original source file ``my_module.cpp``:

   .. code-block:: c++

      #include "my_module.wrap.cxx"

   This effectively injects the generated bindings into the module.

3. Uses `clair-c2py` (which is based on Clang) to parse the C++ code. Compiler options such as include paths and definitions must be provided,
   either explicitly or through a ``compile_commands.json`` file.

   Example usage:

   .. code-block:: bash

      clair-c2py module_source_file.cpp -- all compiler options   # pass options on the command line, after the `--` separator
      clair-c2py module_source_file.cpp                           # uses compile_commands.json in the current directory
      clair-c2py module_source_file.cpp -p DIR                    # uses compile_commands.json from a specified directory DIR

   Notes:

   * In the example above, we assume that ``c2py`` is installed and that ``c2py_flags`` is available in the system path.
     The command ``c2py_flags -i`` provides all necessary include paths for Python.

   * In a CMake project, we typically rely on ``compile_commands.json`` in conjunction with automatic detection of Python and c2py targets.
     (See :ref:`cmake` for more details.)


Compilation
...........   
The second command   

  .. code-block:: bash
   
     clang++ my_module.cpp -std=c++20 -shared -o my_module.so `c2py_flags` 

  compiles the Python C++ extension. Note that:
  
    #. `c2py_flags -i` yields the include and links path for Python. 
    
  .. note::
   
     | Any C++20 compiler can be used to compile the bindings (clang, gcc, etc.).
     | It is independent of the ``clair_c2py`` tool (and on the LLVM/clang version it is based on)..


Documentation
.............

``clair-c2py`` automatically generates the Python documentation from the C++ code comments,
using the standard `numpydoc` format.

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


What is a binding code exactly ?
................................

In order to call C++ from Python, some **binding code** has to be generated.
It is a piece of C++ code which *adapts* the C++ functions and classes to the C API of Python.

The ``clair_c2py`` tool automatizes this task, as it:

#. Parses `my_module.cpp` as usual (the first step of a compilation: check syntax and grammar, build the Abstract Syntax Tree or AST).
#. Generates the C++/Python bindings (by analyzing the AST).

.. note::

  Even though the bindings are readable C++ code, they are designed to be automatically generated, not written by hand.
  The ``c2py`` API is therefore not part of the user documentation.

.. _use_clair_c2py_manually:

Use clair-c2py manually
=======================

The following two commands generate the Python bindings and compile the corresponding Python module for our :ref:`getting_started` example:

.. code-block:: bash
   
   clair-c2py getting_started.cpp -- -std=c++20 `c2py_flags -i`
   clang++ getting_started.cpp -std=c++20 -shared -o getting_started.so `c2py_flags`
   
That's it. The Python module ``getting_started`` is ready to be used.

Let us break down the commands in more detail.

Generate Python Bindings
........................

.. code-block:: bash
   
   clair-c2py getting_started.cpp -- -std=c++20 `c2py_flags -i`

This generates the C++/Python binding files ``getting_started.wrap.cxx`` and ``getting_started.wrap.hxx``, and updates the 
original source file ``getting_started.cpp`` to include the generated bindings. 

See :ref:`generate_python_bindings_and_compile` for some more background information.

.. note::

   * Here, we assume that ``c2py`` is installed and that ``c2py_flags`` is available in the system path.
     The command ``c2py_flags -i`` provides all necessary include paths for Python.

   * In a CMake project, we typically rely on ``compile_commands.json`` in conjunction with automatic detection of Python and 
     **c2py** targets (see :ref:`use_cmake_integration` for more details).

Compile the Module
..................

.. code-block:: bash
   
   clang++ getting_started.cpp -std=c++20 -shared -o getting_started.so `c2py_flags` 

This compiles the Python C++ extension.

.. note::

   * The command ``c2py_flags`` provides all necessary include and linker paths for Python.

   * Any C++20 compiler can be used to compile the bindings (clang, gcc, etc.).
     It is independent of the ``clair_c2py`` tool and of the LLVM/clang version it is based on.

General clair-c2py usage
........................

.. code-block:: console

   USAGE: clair-c2py [options] <source0> [... <sourceN>]

   OPTIONS:

   :

   --extra-arg=<string>        - Additional argument to append to the compiler command line
   --extra-arg-before=<string> - Additional argument to prepend to the compiler command line
   --gen-default-config        - Generate a default TOML configuration file for each source file.
   --generate-depfile=<string> - Generates the depfile for CMake (encodes the dependencies of the bindings)
   -p <string>                 - Build path
   --update-config             - Update the TOML configuration file for each source file.
   -v                          - Verbose

   Generic Options:

   --help                      - Display available options (--help-hidden for more)
   --help-list                 - Display list of available options (--help-list-hidden for more)
   --version                   - Display the version of this program

   clang-c2py generates Python binding for C++.
   Usage:
      clair-c2py module_source_file.cpp -- all compiler options   # pass options on the command line, after the `--` separator
      clair-c2py module_source_file.cpp                           # uses compile_commands.json in the current directory
      clair-c2py module_source_file.cpp -p DIR                    # uses compile_commands.json from a specified directory DIR

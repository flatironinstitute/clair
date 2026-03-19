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

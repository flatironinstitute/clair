.. _examples:

Examples
********
    
.. toctree::
   :maxdepth: 2
   :hidden:


Functions
========

The following example shows a C++ function that takes a ``std::vector<double>`` and returns the sum of its elements:

.. literalinclude:: examples/fun2.cpp
   :language: cpp

We can then generate the Python bindings for this function and compile them with (assuming OS X and clang):

.. code-block:: bash
   
   $ clang++ -fplugin=clair_c2py.dylib  fun2.cpp -std=c++20 -shared -o fun2.so `c2py_flags`

Finally, we use the module in Python:

.. code-block:: console

    >>> import fun2 as M
    >>> M.sum([1.2, 2.3, 4.5])
    8.0
    
Classes
=======

The following example shows a simple C++ struct and how to make it printable in Python:

.. literalinclude:: examples/struct1.cpp
   :language: cpp

We can again generate the Python bindings for this function and compile them with (assuming OS X and clang):

.. code-block:: bash
   
   $ clang++ -fplugin=clair_c2py.dylib  struct1.cpp -std=c++20 -shared -o struct1.so `c2py_flags`

Finally, we use the module in Python:

.. code-block:: console

    >>> import struct1 as M
    >>> s = M.S(2)
    >>> s.i
    2
    >>> s.m()
    2
    >>> print(s)
    S struct with i=2
    >>> M.f(s)
    2


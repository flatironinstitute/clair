.. _functions:

Functions
*********

The following example shows a C++ function that takes a ``std::vector<double>`` and returns the sum of its elements:

.. literalinclude:: ./fun2.cpp
   :language: cpp

We can then generate the Python bindings for this function and compile them with (assuming OS X and clang):

.. code-block:: bash
   
    clair-c2py fun2.cpp -- -std=c++20 `c2py_flags -i`
    clang++ fun2.cpp -std=c++20 -shared -o fun2.so `c2py_flags`

Finally, we use the module in Python:

.. code-block:: console

    >>> import fun2 as M
    >>> M.sum([1.2, 2.3, 4.5])
    8.0

.. _classes:

Classes
*******

The following example shows a simple C++ struct and how to make it printable in Python:

.. literalinclude:: ./struct1.cpp
   :language: cpp

We can again generate the Python bindings for this function and compile them with (assuming OS X and clang):

.. code-block:: bash
   
     clair-c2py struct1.cpp -- -std=c++20 `c2py_flags -i`
     clang++ struct1.cpp -std=c++20 -shared -o struct1.so `c2py_flags`

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

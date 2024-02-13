.. _examples:

Examples
********
    
.. toctree::
   :maxdepth: 2
   :hidden:


Function
========


.. literalinclude:: examples/fun2.cpp
   :language: cpp
   :linenos:

.. code-block:: console

    >>> import fun2 as M
    >>> M.sum([1.2, 2.3, 4.5])
    8.0
    
Classes
=======


.. literalinclude:: examples/struct1.cpp
   :language: cpp
   :linenos:

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


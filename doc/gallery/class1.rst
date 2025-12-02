.. _classes:

Classes
*******

This example demonstrates a small C++ class ``S`` with:

   - A data member and a method
   - A free function: ``f`` using ``S``
   - The C++ printing automatically being used in Python.

.. literalinclude:: ../examples/gallery/class1.cpp
   :language: cpp
   :caption: class1.cpp
   :end-before: #include "class1.wrap.cxx"

In Python, we have

.. testsetup::

   import sys, os
   sys.path.insert(0, os.path.abspath('examples/gallery'))

.. doctest::

   >>> import class1 as M
   >>> s = M.S(2)
   >>> s.i
   2
   >>> s.m()
   4
   >>> print(s)
   S struct with i=2
   >>> M.f(s)
   2

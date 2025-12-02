.. _functions:

Functions
*********

A function using some STL containers and algorithm.


.. literalinclude:: ../function_w_stl.cpp
   :language: cpp
   :caption: function_w_stl.cpp
   :end-before: #include "function_w_stl.wrap.cxx"

In Python, we get:

.. testsetup::

   import sys, os
   sys.path.insert(0, os.path.abspath('examples'))

.. doctest::

   >>> import function_w_stl as M
   >>> M.sum([1.2, 2.3, 4.5])
   8.0

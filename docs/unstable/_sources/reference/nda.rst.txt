TRIQS/nda 
==========

The `nda <https://github.com/TRIQS/nda>`_ library 
set up to work with `clair/c2py` automatically.
``nda`` provides N-dimensional arrays and views for C++20.
They are converted to/from `numpy <https://numpy.org/>`_ arrays in Python.
Here is an example:

.. warning::
   
   TBW: discuss the ownership of array/view with python
   
.. literalinclude:: ../examples/nda/nda_example1.cpp
	:language: cpp
	:caption: nda_example1.cpp
	:end-before: #include "nda_example1.wrap.cxx"


.. literalinclude:: ../examples/nda/CMakeLists.txt  
   :language: cmake
   :caption: CMakeLists.txt
   :lines: 1, 3-6

Python usage:

.. testsetup::

   import sys, os
   sys.path.insert(0, os.path.abspath('examples/nda'))

.. doctest::

    >>> from nda_example1 import my_sum, make_array
    >>> import numpy as np
    >>> a = np.array([1,2,3], dtype = np.float64)
    >>> my_sum(a)
    6.0
    >>> make_array(5)
    array([5, 6], dtype=int32)



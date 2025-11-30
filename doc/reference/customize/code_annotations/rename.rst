:orphan:

.. _example_rename:

C2PY_RENAME
***********

The ``C2PY_RENAME`` annotation can be used to give a wrapped function or class a different name in the Python bindings.

C++ code
--------

.. literalinclude:: ../../../examples/code_annotations/c2py_rename.cpp
    :language: cpp
    :end-before: #include "c2py_rename.wrap.cxx"


Usage in Python
---------------

After generating the extension module, we can use it in Python:

.. testsetup::

   import sys, os
   sys.path.insert(0, os.path.abspath('examples/code_annotations'))


.. doctest::

    >>> from c2py_rename import *
    >>> g(5)
    10
    >>> f(5)
    Traceback (most recent call last):
    ...
    NameError: name 'f' is not defined

The function ``f`` has been renamed to ``g`` as instructed.

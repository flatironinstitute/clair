:orphan:

.. _example_ignore:

C2PY_IGNORE
***********

The ``C2PY_IGNORE`` annotation can be used to exclude specific functions or classes from being exposed in the Python bindings.

C++ code
--------

.. literalinclude:: ../../../examples/code_annotations/c2py_ignore.cpp
   :language: cpp

Usage in Python
---------------

After generating the extension module, we can use it in Python:

.. code-block:: console

    >>> from c2py_ignore import *
    >>> g(2)
    6
    >>> f(2)
    Traceback (most recent call last):
      File "<python-input-2>", line 1, in <module>
        f(2)
        ^
    NameError: name 'f' is not defined

As expected, only the function ``g`` is available in Python, while ``f`` has been ignored.

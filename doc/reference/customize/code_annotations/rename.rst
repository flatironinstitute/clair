:orphan:

.. _example_rename:

C2PY_RENAME
***********

The ``C2PY_RENAME`` annotation can be used to give a wrapped function or class a different name in the Python bindings.

C++ code
--------

.. literalinclude:: ../../../examples/code_annotations/c2py_rename.cpp
   :language: cpp

Usage in Python
---------------

After generating the extension module, we can use it in Python:

.. code-block:: console

    >>> from c2py_rename import *
    >>> g(5)
    10
    >>> f(5)
    Traceback (most recent call last):
    File "<python-input-2>", line 1, in <module>
        f(5)
        ^
    NameError: name 'f' is not defined

The function ``f`` has been renamed to ``g`` as instructed.

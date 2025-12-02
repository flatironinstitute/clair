:orphan:

.. _example_wrap_as_method:

C2PY_WRAP_AS_METHOD
*******************

The ``C2PY_WRAP_AS_METHOD`` annotation lets us add a function as a method to the first argument's class object.

C++ code
--------

.. literalinclude:: ../../../examples/code_annotations/c2py_wrap_as_method.cpp
   :language: cpp

Usage in Python
---------------

After generating the extension module, we can use it in Python:

.. code-block:: console

    >>> from c2py_wrap_as_method import *
    >>> m = Myclass(x = 100)
    >>> m.f(5)
    500
    >>> f(5)
    Traceback (most recent call last):
    File "<python-input-5>", line 1, in <module>
        f(5)
        ^
    NameError: name 'f' is not defined

The free C++ function ``f`` has been added as a method to the ``Myclass`` class in Python.

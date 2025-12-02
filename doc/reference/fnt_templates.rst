.. _function_templates:

Function templates
******************

In the case of generic (template) functions, 
clair-c2py generate bindings only for 
their **explicit instantiations**.

Example
=======

.. literalinclude:: ../examples/function_template.cpp
   :language: cpp
   :caption: function_template.cpp

Here, we define a function template ``add`` that adds two values of the same type.
Then we explicitly instantiate the template for the types ``int``, ``double``, and ``std::string``.

Usage in Python
===============

.. testsetup::

   import sys
   import os
   # Add the examples build directory to Python path
   # This assumes doctest is run from the build/doc directory
   sys.path.insert(0, os.path.abspath('../examples'))

.. doctest::

    >>> from function_template import add
    >>> add(1, 2)
    3
    >>> add(2.0, 5.0)
    7.0
    >>> add("Hello, ", "world!")
    'Hello, world!'
    >>> add(1, 2.0)
    3.0

**clair** dispatches the function calls to the appropriate instantiation based on the argument types.
Adding two integers, doubles or strings works as expected.

Error handling
==============

In case we pass two incompatible types, the error message
is similar to any other dynamical dispatch failure.

.. doctest::

    >>> add(1, "hello")
    Traceback (most recent call last):
      ...
    TypeError: [c2py] Can not call the function with the arguments
       (1, 'hello')
    The dispatch to C++ failed with the following error(s):
    [1] (a: int, b: int)
        -> int
        -- b: Cannot convert hello to integer type
    <BLANKLINE>
    [2] (a: float, b: float)
        -> float
        -- b: Cannot convert <class 'str'> to double
    <BLANKLINE>
    [3] (a: str, b: str)
        -> str
        -- a: Cannot convert 1 to string
    <BLANKLINE>
    <BLANKLINE>

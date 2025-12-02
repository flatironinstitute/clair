.. _fntref:

Functions
*********


Type conversion
---------------

Python and C++ types are different. 
For example, a ``long`` in C++ is converted from/to a Python integer.
In order to call a C++ function from Python, it is necessary to

* convert its arguments to C++,
* call it and
* convert its return value to Python.

Hence, only functions whose arguments and return value are of **convertible types** can be called across languages.
``clair`` will report a compilation error for any attempt to wrap a function with an inconvertible type.


Dynamical dispatch
------------------

* In C++, objects are statically typed (one variable has one type fixed at compilation), and functions can be overloaded. 
* In Python, objects are dynamically typed, and there is no overload.

As a result, several functions in C++ will typically be gathered into one Python function.

When the Python function is called, the number and types of its arguments are analyzed (at runtime), 
and the first C++ function for which the Python arguments can be converted to the C++ types is called.
If none applies, a Python exception is raised.

Let us illustrate this with a simple example.

.. literalinclude:: ../examples/fun1.cpp
   :language: cpp
   :caption: fun1.cpp
   :end-before: #include "fun1.wrap.cxx"

clair-c2py reports:

.. code-block:: console

    -- Function: f
    --         . f(int x)
    --         . f(int x, int y)
   
meaning that the two C++ overloads of ``f`` are "gathered" in one Python function ``f``.

.. testsetup::

   import sys
   import os
   # Add the examples build directory to Python path
   # Doctest runs from build/doc, examples are in build/doc/examples
   sys.path.insert(0, os.path.abspath('examples'))

.. doctest::

   >>> import fun1 as M
   >>> M.f(1)
   -1
   >>> M.f(1,2)
   3

If no dispatch is possible for the arguments given, ``c2py`` reports a ``TypeError``, e.g.

.. doctest::

    >>> M.f(1,2,3)
    Traceback (most recent call last):
      ...
    TypeError: [c2py] Can not call the function with the arguments
       (1, 2, 3)
    The dispatch to C++ failed with the following error(s):
    [1] (x: int)
        -> int
        -- Too many arguments. Expected at most 1 and got 3
    <BLANKLINE>
    [2] (x: int, y: int)
        -> int
        -- Too many arguments. Expected at most 2 and got 3
    <BLANKLINE>
    <BLANKLINE>

or 

.. doctest::

    >>> import fun1 as M
    >>> M.f("abc")
    Traceback (most recent call last):
      ...
    TypeError: [c2py] Can not call the function with the arguments
       ('abc',)
    The dispatch to C++ failed with the following error(s):
    [1] (x: int)
        -> int
        -- x: Cannot convert abc to integer type
    <BLANKLINE>
    [2] (x: int, y: int)
        -> int
        -- Too few arguments. Expected at least 2 and got 1
    <BLANKLINE>
    <BLANKLINE>


Template functions
------------------

The instantiation of generic functions is discussed in a :ref:`separate section <function_templates>`.

.. toctree::
   :maxdepth: 1

   fnt_templates


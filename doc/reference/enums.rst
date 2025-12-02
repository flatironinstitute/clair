.. _enum:

Enumerations
************

**clair-c2py** automatically wraps C++ enumerations (both ``enum`` and ``enum class``) as Python string-based enumerations.

C++ Code
========

Define your enumerations in C++:

**my_enums.cpp**:

.. literalinclude:: ../examples/my_enums.cpp
   :language: cpp

Python Usage
============

.. testsetup::

   import sys, os
   sys.path.insert(0, os.path.abspath('examples'))

In Python, enumeration values are represented as strings:

.. doctest::

   >>> import my_enums as M
   >>> M.f1("a")
   'a'
   >>> M.f2("A")
   'A'

Invalid values raise ``TypeError``:

.. doctest::

   >>> M.f1("AA")
   Traceback (most recent call last):
   ...
   TypeError: ...
   ...
   The string "AA" is not in a,b,c
   <BLANKLINE>
   <BLANKLINE>




.. _enum:

Enumerations
************

``clair-c2py`` automatically wraps C++ enumerations 
(both ``enum`` and ``enum class``) as Python strings, 
subject to the same customization rules as classes and functions 
(see :ref:`customize`).

Let us show a simple example with two enumerations and functions that use them:

.. literalinclude:: ../examples/my_enums.cpp
   :language: cpp
   :caption: my_enums.cpp
   :end-before: #include "my_enums.wrap.cxx"

In Python, we get:

.. testsetup::

   import sys, os
   sys.path.insert(0, os.path.abspath('examples'))

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




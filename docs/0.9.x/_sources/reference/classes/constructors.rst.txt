.. _class_constructors:

Constructors
============

#. ``clair-c2py`` wraps all public constructors of a class.

#. For C++ aggregates without user-defined constructors
   (i.e., structs with public fields only), 
   ``clair-c2py`` synthesizes a constructor from a Python dict, 
   mimicking in Python the C++ aggregate initialization syntax.

User-defined constructors
-------------------------

Let us first illustrate user-defined constructors with a simple example:   

.. literalinclude:: ../../examples/classes/constructor_basic.cpp
   :language: cpp
   :caption: constructor_basic.cpp
   :end-before: #include "constructor_basic.wrap.cxx"

In Python, we get:

.. testsetup::

   import sys, os
   sys.path.insert(0, os.path.abspath('examples/classes'))

.. doctest::

   >>> from constructor_basic import Point
   >>> p1 = Point()              # Default constructor
   >>> p2 = Point(3.0, 4.0)      # Constructor with arguments
   >>> p3 = Point(x=1.0, y=2.0)  # Keyword arguments
   >>> print(p1.x(), p1.y())
   0.0 0.0


Synthesized aggregate constructor
---------------------------------

The second example illustrates the aggregate constructor:

.. literalinclude:: ../../examples/classes/constructor_aggregate.cpp
   :language: cpp
   :caption: constructor_aggregate.cpp
   :end-before: #include "constructor_aggregate.wrap.cxx"

In Python, we get:

.. doctest::

   >>> from constructor_aggregate import Params
   >>> p = Params(width=800, height=600, fullscreen=True, title="My App")

The synthesized constructor accepts only keyword arguments and performs the following checks:

- All fields without a default value must be initialized.
- No unknown fields are allowed.

If any of these checks fail, a ``RuntimeError`` is raised.

.. doctest::

   >>> p = Params(height=600, fullscreen=True)
   Traceback (most recent call last):
     ...
   RuntimeError: 
    Mandatory parameter 'width' is missing.

   >>> p = Params(width=800, height=600, fullscreen=True, title="My App", extra=42)
   Traceback (most recent call last):
     ...
   RuntimeError: 
    The parameter 'extra' was not used.

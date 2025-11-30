.. _class_constructors:

Constructors
************

Clair wraps all public constructors of a class. Arguments with default values are properly handled,
and keyword arguments are supported in Python.

Basic Constructor
=================

.. literalinclude:: ../../examples/classes/constructor_basic.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import Point
   p1 = Point()           # Default constructor
   p2 = Point(3.0, 4.0)   # Constructor with arguments
   p3 = Point(x=1.0, y=2.0)  # Keyword arguments


Synthesized Constructors from Parameters
=========================================

For classes with many parameters, clair can synthesize a constructor that takes a parameter struct,
automatically converting Python keyword arguments to the struct members.

.. literalinclude:: ../../examples/classes/constructor_params.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import Config
   # All parameters as keyword arguments
   cfg = Config(width=800, height=600, fullscreen=True, title="My App")

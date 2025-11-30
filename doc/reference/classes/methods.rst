.. _class_methods:

Methods
*******

Instance Methods
================

All public methods are wrapped and callable from Python. Overloaded methods are automatically dispatched
based on argument types.

.. literalinclude:: ../../examples/classes/methods_basic.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import Rectangle
   r = Rectangle(10.0, 20.0)
   print(r.area())        # 200.0
   r.scale(2.0)
   print(r.area())        # 800.0


Const Methods
=============

Const methods are properly wrapped and can be called on const objects.

.. literalinclude:: ../../examples/classes/methods_const.cpp
   :language: cpp


Static Methods
==============

Static methods are exposed as class methods in Python.

.. literalinclude:: ../../examples/classes/static_methods.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import MathUtils
   result = MathUtils.add(3, 5)  # Call without instance
   print(result)  # 8

.. _class_operators:

Operators
*********

Operator()
==========

The function call operator allows objects to be called like functions in Python.

.. literalinclude:: ../../examples/classes/operator_call.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import Multiplier
   mult = Multiplier(5)
   result = mult(10)  # Calls operator()
   print(result)  # 50


Operator[]
==========

The subscript operator enables indexing and item access in Python.

.. literalinclude:: ../../examples/classes/operator_subscript.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import Container
   c = Container(10)
   c[0] = 42
   print(c[0])  # 42


Arithmetic Operators
====================

Arithmetic operators (+, -, *, /, etc.) are automatically wrapped.

.. literalinclude:: ../../examples/classes/operators.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import Vector
   v1 = Vector(1.0, 2.0)
   v2 = Vector(3.0, 4.0)
   v3 = v1 + v2
   print(v3.x, v3.y)  # 4.0, 6.0


Comparison Operators
====================

Comparison operators (==, !=, <, >, <=, >=) are wrapped when defined.

.. literalinclude:: ../../examples/classes/comparison.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import Version
   v1 = Version(1, 0, 0)
   v2 = Version(2, 0, 0)
   print(v1 < v2)   # True
   print(v1 == v2)  # False

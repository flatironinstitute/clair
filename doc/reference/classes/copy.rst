.. _class_copy:

Copy Semantics
**************

Classes with copy constructors are automatically copyable in Python.

.. literalinclude:: ../../examples/classes/copy.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import Data
   import copy
   
   d1 = Data(42)
   d2 = copy.copy(d1)   # Shallow copy
   d3 = copy.deepcopy(d1)  # Deep copy

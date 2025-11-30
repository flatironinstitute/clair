.. _class_iterable:

Iterable Objects
****************

Classes that provide ``begin()`` and ``end()`` methods are automatically iterable in Python.

.. literalinclude:: ../../examples/classes/iterable.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import Range
   r = Range(5)
   for i in r:
       print(i)  # 0, 1, 2, 3, 4

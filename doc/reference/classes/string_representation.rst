.. _class_string_representation:

String Representation
*********************

The ``__str__`` and ``__repr__`` methods can be customized for Python.

.. literalinclude:: ../../examples/classes/str_repr.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import Person
   p = Person("Alice", 30)
   print(p)        # Uses __str__: "Alice (30 years old)"
   print(repr(p))  # Uses __repr__: "Person('Alice', 30)"

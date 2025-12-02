:orphan:

.. _example_module_init:

C2PY_MODULE_INIT
****************

The ``C2PY_MODULE_INIT`` annotation lets us define a function that will be called when the module is initialized/imported.

C++ code
--------

.. literalinclude:: ../../../examples/code_annotations/c2py_module_init.cpp
   :language: cpp

Usage in Python
---------------

After generating the extension module, we can use it in Python:

.. code-block:: console

    >>> import c2py_module_init
    c2py/clair rocks!

Importing the module triggers the execution of the ``init`` function, which prints a message to the console.

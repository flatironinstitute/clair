.. _class_templates:

Class templates
***************

Here we demonstrate how to generate Python bindings class templates.

In contrast to :ref:`function_templates`, class templates need to be instanstiated in the special 
``c2py_module`` namespace with a ``using`` declaration.

.. literalinclude:: ./class_template.cpp
    :language: cpp

**clair** performs the actual instantiation for us and gives it the desired Python name written on 
the left-hand side of the ``using`` declaration.

After creating the Python bindings,

.. code-block:: bash
   
     clair-c2py class_template.cpp -- -std=c++20 `c2py_flags -i`
     clang++ class_template.cpp -std=c++20 -shared -o class_template.so `c2py_flags`

we can test the extension module in Python:

.. code-block:: console

    >>> from class_template import *
    >>> a_i = int_adder(2)
    >>> a_i.add(5)
    7
    >>> a_d = double_adder(3.14)
    >>> a_d.add(5.7)
    8.84
    >>> a_s = string_adder("Hello ,")
    >>> a_s.add("clair!")
    'Hello ,clair!'


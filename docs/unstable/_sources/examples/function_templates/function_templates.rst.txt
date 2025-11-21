.. _function_templates:

Function templates
******************

Here we demonstrate how to generate Python bindings for function templates.

Function templates can be wrapped by explicitly instantiating them:

.. literalinclude:: ./function_template.cpp
   :language: cpp

Here, we define a function template ``add`` that adds two values of the same type.
Then we explicitly instantiate the template for the types ``int``, ``double``, and ``std::string``.

To generate the Python bindings, we follow the usual procedure:

.. code-block:: bash
   
     clair-c2py function_template.cpp -- -std=c++20 `c2py_flags -i`
     clang++ function_template.cpp -std=c++20 -shared -o function_template.so `c2py_flags`

We can then test the generated bindings in Python:

.. code-block:: console

    >>> from function_template import add
    >>> add(1, 2)
    3
    >>> add(2.0, 5.0)
    7.0
    >>> add("Hello, ", "world!")
    'Hello, world!'
    >>> add(1, 2.0)
    3.0

**clair** dispatches the function calls to the appropriate instantiation based on the argument types.
Adding two integers, doubles or strings works as expected.

In case we pass two incompatible types, an informative error message is raised:

.. code-block:: console

    >>> add(1, "hello")
    Traceback (most recent call last):
    File "<python-input-7>", line 1, in <module>
        add(1, "hello")
        ~~~^^^^^^^^^^^^
    TypeError: [c2py] Can not call the function with the arguments
    (1, 'hello')
    The dispatch to C++ failed with the following error(s):
    [1] (a: int, b: int)
        -> int
        -- b: Cannot convert hello to integer type

    [2] (a: float, b: float)
        -> float
        -- b: Cannot convert <class 'str'> to double

    [3] (a: str, b: str)
        -> str
        -- a: Cannot convert 1 to string


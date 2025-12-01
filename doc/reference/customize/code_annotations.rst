.. _code_annotations:

Code annotations
****************

``clair``'s behaviour can be modified by simple annotations to functions or classes
directly in the C++ source code.
These macros are simply short-cuts for C++ annotations, 
which are ignored by the C++ compiler, but picked up by ``clair`` during the bindings generation.


.. list-table::
    :header-rows: 1
    :widths: 35 65

    * - Annotation
      - Description
    * - ``C2PY_IGNORE``
      - Ignore the function/class.
      
        :ref:`Example <example_ignore>`
    * - ``C2PY_RENAME(new_name_in_python)``
      - Rename the function/class. Overrules the default name deduction.
      
        :ref:`Example <example_rename>`
    * - ``C2PY_WRAP_AS_METHOD``
      - Transform a function into a method to the first argument's class.
      
        :ref:`Example <example_wrap_as_method>`
    * - ``C2PY_MODULE_INIT``
      - Mark a function to be called when the module is imported.
      
        :ref:`Example <example_module_init>`

.. note::
    These annotations overrule any options in the TOML file (Cf :ref:`toml_options`).


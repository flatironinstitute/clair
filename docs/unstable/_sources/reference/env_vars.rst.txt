.. _env_vars:

Environment Variables
*********************

``clair-c2py`` reads several environment variables to control its runtime behaviour.

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Variable
     - Description
   * - ``CLAIR_VERBOSE``
     - Verbosity level for diagnostic output (see :ref:`below <clair_verbose>`).
   * - ``CLAIR_SKIP_CLANG_FORMAT``
     - Disable automatic ``clang-format`` of generated files (see :ref:`below <clair_skip_clang_format>`).
   * - ``CLICOLOR_FORCE`` / ``LLVM_FORCE_COLOR``
     - Force coloured Clang diagnostics even when output is redirected.
   * - ``SDKROOT`` *(macOS only)*
     - Override the macOS SDK root path used by the compiler.

----

.. _clair_verbose:

CLAIR_VERBOSE
-------------

``CLAIR_VERBOSE`` controls how much diagnostic output ``clair-c2py`` writes to the screen.
It must be set to a non-negative integer. The default is ``0``.

**Screen output** is shown on ``stderr`` and is gated by a per-logger verbosity level:
a message with verbosity *V* is printed to the screen only when ``CLAIR_VERBOSE >= V``.

**Log file output** is independent of ``CLAIR_VERBOSE``: every run creates a file
``<module>.log`` in the same directory as the source file, and all logger output is
written to it unconditionally.

.. warning::

   The log file contains ANSI colour codes. Opening it with a plain text editor
   or ``cat`` will show raw escape sequences. Use ``less -R`` to render colours
   correctly:

   .. code-block:: bash

      less -R mymodule.log

Verbosity levels in use:

.. list-table::
   :widths: 15 85
   :header-rows: 1

   * - Level
     - What is shown on screen
   * - ``0`` (default)
     - Errors, warnings, and configuration errors only.
   * - ``1``
     - The above, plus a summary of every wrapped entity (modules, classes,
       functions, methods, enums, properties) and rejection notices.
   * - ``2``
     - The above, plus per-overload detail for every dispatched function or method.

Usage examples:

.. code-block:: bash

   # Silent on screen; mymodule.log is always written
   clair-c2py mymodule.cpp -- -std=c++20

   # Show wrapped entities on screen
   CLAIR_VERBOSE=1 clair-c2py mymodule.cpp -- -std=c++20

   # Show per-overload detail
   CLAIR_VERBOSE=2 clair-c2py mymodule.cpp -- -std=c++20

   # Inspect the full log after any run
   less -R mymodule.log

----

.. _clair_skip_clang_format:

CLAIR_SKIP_CLANG_FORMAT
------------------------

By default, ``clair-c2py`` runs ``clang-format`` on the generated ``<module>.wrap.cxx``
and ``<module>.wrap.hxx`` files before writing them to disk.
Setting ``CLAIR_SKIP_CLANG_FORMAT`` to any value disables this step:

.. code-block:: bash

   CLAIR_SKIP_CLANG_FORMAT=1 clair-c2py mymodule.cpp -- -std=c++20

This can be useful to speed up iterative development or when ``clang-format`` is
not available in the build environment.

----

CLICOLOR_FORCE / LLVM_FORCE_COLOR
----------------------------------

Clang normally disables colour diagnostics when its output is redirected to a pipe
or file. Setting either ``CLICOLOR_FORCE`` or ``LLVM_FORCE_COLOR`` to any non-empty
value forces colour output regardless:

.. code-block:: bash

   CLICOLOR_FORCE=1 clair-c2py mymodule.cpp -- -std=c++20 2>&1 | less -R

----

SDKROOT *(macOS only)*
-----------------------

On macOS, ``clair-c2py`` needs the path to the active SDK. If ``SDKROOT`` is not set,
it is auto-detected via ``xcrun --show-sdk-path`` and set for the current process.
If it is set but does not match the value returned by ``xcrun``, a warning is emitted.
This variable is ignored on Linux.

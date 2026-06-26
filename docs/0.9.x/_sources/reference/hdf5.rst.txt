.. _class_hdf5:


HDF5 (TRIQS/h5 Integration)
***************************

clair/c2py integrates with the TRIQS/h5 library, enabling serialization and deserialization of C++ classes to HDF5 files from Python.

If your C++ class implements HDF5 support via TRIQS/h5 (the ``h5_write`` / ``h5_read`` friend functions), clair automatically makes the wrapped objects usable with the TRIQS/h5 ``HDFArchive``.

**C++ Example:**

.. literalinclude:: ../examples/classes/hdf5.cpp
   :language: cpp

**Python Usage:**

The wrapped type can be stored in and retrieved from a TRIQS/h5 ``HDFArchive``:

.. code-block:: python

   from h5 import HDFArchive
   from hdf5 import Matrix          # module 'hdf5', class mylib::matrix -> Matrix

   m = Matrix(3, 3)

   # Write to an HDF5 archive
   with HDFArchive("data.h5", "w") as ar:
       ar["matrix"] = m

   # Read it back
   with HDFArchive("data.h5", "r") as ar:
       m2 = ar["matrix"]

**Notes:**

- The HDF5 interface relies on the TRIQS/h5 C++ library for the serialization logic (the ``h5_write`` / ``h5_read`` friends shown above).
- For each class with HDF5 support, clair generates a ``__write_hdf5__`` method and registers the type with the ``h5.formats`` registry, so it works transparently with the TRIQS/h5 ``HDFArchive``.

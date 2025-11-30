.. _class_hdf5:


HDF5 (TRIQS/h5 Integration)
***************************

clair/c2py provides seamless integration with the TRIQS/h5 library, enabling automatic serialization and deserialization of C++ classes to HDF5 files from Python.

If your C++ class implements HDF5 support via TRIQS/h5, clair will automatically expose corresponding Python methods for saving and loading objects.

**C++ Example:**

.. literalinclude:: ../examples/classes/hdf5.cpp
   :language: cpp

**Python Usage:**

.. code-block:: python

   from mymodule import Matrix
   import h5py

   m = Matrix(3, 3)
   # Save to HDF5
   with h5py.File('data.h5', 'w') as f:
       m.save_to_hdf5(f, 'matrix')

   # Load from HDF5
   with h5py.File('data.h5', 'r') as f:
       m2 = Matrix.load_from_hdf5(f, 'matrix')

**Notes:**

- The HDF5 interface relies on the TRIQS/h5 C++ library for serialization logic.
- Python bindings automatically mirror the C++ API, making it easy to persist and restore objects.
- You can use any HDF5-compatible Python library (e.g., h5py) for file management.

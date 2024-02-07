source ~/c2py_install/share/c2pyvars.sh
source ~/clair_install/share/clair/clairvars.sh
clang++ -fplugin=clair_c2py.dylib `c2py_flags` ${1}.cpp -std=c++20 -shared -o ${1}.so


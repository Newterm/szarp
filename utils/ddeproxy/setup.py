# py2exe setup file for ddeproxy
# run something like:
#  python setup.py py2exe
#
# If everything works well, you should find a subdirectory named 'dist'
# containing output files.

from distutils.core import setup
import py2exe

setup(
    # The first three parameters are not required, if at least a
    # 'version' is given, then a versioninfo resource is built from
    # them and added to the executables.
    version = "0.1",
    description = "SZARP DDE Proxy",
    name = "ddeproxy",

    # targets to build
    windows = ["ddeproxy.py"],
    )

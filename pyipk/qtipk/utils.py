
from PyQt4 import QtCore

try:
	_fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    _fromUtf8 = lambda s: s.decode('utf8')

try:
	_toUtf8 = lambda s : unicode(QtCore.QString.toUtf8(s))
except AttributeError:
	_toUtf8 = lambda s: s


fromUtf8 = _fromUtf8
toUtf8 = _toUtf8


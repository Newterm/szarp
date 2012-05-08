import sip
sip.setapi('QString', 2)

from PyQt4 import QtCore , QtGui

_toUtf8 = lambda s: s.decode('utf8')
_fromUtf8 = lambda s : s.encode('utf8')

fromUtf8 = _fromUtf8
toUtf8 = _toUtf8

stdIcon = QtGui.QIcon.fromTheme


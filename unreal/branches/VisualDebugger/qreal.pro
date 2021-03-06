win32-msvc* {
	CONFIG(debug, debug|release) {
		QRXC = qrxc\\debug\\qrxc.exe
	} else:CONFIG(release, debug|release){
		QRXC = qrxc\\release\\qrxc.exe
	}
	!exists($$QRXC) {
		COMMAND = $$escape_expand(\"C:\Program Files\Microsoft Visual Studio 9.0\Common7\Tools\vsvars32.bat\") && cd qrxc && qmake -tp vc && vcbuild
		SYS = $$system($$COMMAND)
	}
}

contains(QT_VERSION, ^4\\.[0-5]\\..*){
	message("Cannot build qreal using Qt version $${QT_VERSION}")
	error("Use at least Qt 4.6")
}

TEMPLATE	= subdirs
SUBDIRS		= qrxc qrgui qrxml qrrepo

qrmc.depends = qrrepo
qrgui.depends = qrxc qrxml qrrepo
qrxml.depends = qrxc

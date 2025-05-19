CC=gcc
LDFLAGS=-pthread $(shell pkg-config --cflags --libs gtk+-3.0 libxml-2.0)
CFLAGS=-g -ggdb -Wall
GTKWRAP_BIN=gtk-wrap

all:
	${CC} ${CFLAGS} gtk-wrap.c -o ${GTKWRAP_BIN} ${LDFLAGS}

clean:
	rm ${GTKWRAP_BIN}

strip:
	strip -s ${GTKWRAP_BIN}


# build package
GIT_VERSION=$(patsubst v%,%,$(shell git describe --tags --long --dirty))
PACKAGE_DEPS=$(patsubst shlibs:Depends=%,%,$(shell dpkg-shlibdeps ${GTKWRAP_BIN} -O))
PACKAGE_TYPE=deb
PACKAGE_ARCH=$(shell dpkg --print-architecture)
PACKAGE_NAME=gtk-wrap
PACKAGE_FILENAME=${PACKAGE_NAME}-${GIT_VERSION}-${PACKAGE_ARCH}.${PACKAGE_TYPE}
PACKAGE_BINDIR=/usr/local/bin
LINTIAN_IGNORE=dir-in-usr-local,file-in-usr-local,no-copyright-file,debian-changelog-file-missing-or-wrong-name,extended-description-is-empty

deb:	all strip

ifeq (, $(shell which dpkg-shlibdeps))
	$(error "dpkg-shlibdeps is required, consider doing apt-get install dpkg-dev")
endif
ifeq (, $(shell which fpm))
	$(error "fpm is required, please check https://fpm.readthedocs.io/en/latest/installation.html")
endif
	fpm -f \
  -s dir -t ${PACKAGE_TYPE} \
  -p ${PACKAGE_FILENAME} \
  --name ${PACKAGE_NAME} \
  --version ${GIT_VERSION} \
  --architecture ${PACKAGE_ARCH} \
  --depends "${PACKAGE_DEPS}" \
  --description "GTK gui in bash." \
  --url "https://github.com/abecadel/gtkwrap" \
  --maintainer "Nicolas Gaullier <nicolas.gaullier@cji.paris>" \
    ${GTKWRAP_BIN}=${PACKAGE_BINDIR}/${GTKWRAP_BIN}
ifeq (, $(shell which lintian))
	@echo "warning: lintian is not available: package checking is disabled, consider doing apt-get install lintian"
else
	lintian --suppress-tags "${LINTIAN_IGNORE}" ${PACKAGE_FILENAME}
endif

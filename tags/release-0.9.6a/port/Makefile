# New ports collection makefile for:	japi
# Date created:						30 January 2009
# Whom:								Maarten L. Hekkelman
#
# $FreeBSD$
#

PORTNAME=		japi
PORTVERSION=	0.9.6a
CATEGORIES=		editors
MASTER_SITES=	http://www.xs4all.nl/~hekkel2/

MAINTAINER=		maarten@hekkelman.com
COMMENT=		A gtk2 based text editor and minimal IDE

BUILD_DEPENDS+=	cryptopp:${PORTSDIR}/security/cryptopp
LIB_DEPENDS+=	glade-2.0:${PORTSDIR}/devel/libglade2 \
				gstreamer-0.10.0:${PORTSDIR}/multimedia/gstreamer \
				boost_thread:${PORTSDIR}/devel/boost \
				boost_filesystem:${PORTSDIR}/devel/boost \
				expat:${PORTSDIR}/textproc/expat2 \
				pcre:${PORTSDIR}/devel/pcre

# MAN1=			japi.1
USE_GMAKE=		yes

PLIST_FILES=	bin/japi \
				share/locale/nl/LC_MESSAGES/japi.mo \
				share/applications/japi.desktop

.include <bsd.port.mk>

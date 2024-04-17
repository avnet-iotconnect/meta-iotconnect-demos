LICENSE = "GPL-3.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-3.0-only;md5=c79ff39f19dfec6d293b95dea7b07891"

RDEPENDS_${PN} = "bash"

SRC_URI = "file://config-data/"

APP_INSTALL_DIR = "${base_prefix}/usr/iotc/local"
FILES_${PN} += "${APP_INSTALL_DIR}/*"

# FILES_${PN}-dev = "${APP_INSTALL_DIR}/*"

do_install() {
    install -d ${D}${APP_INSTALL_DIR}/
    install -d ${D}${APP_INSTALL_DIR}/local

    cp -R --no-preserve=ownership ${WORKDIR}/config-data/* ${D}${APP_INSTALL_DIR}/local/
}

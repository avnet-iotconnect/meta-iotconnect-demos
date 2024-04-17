LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

RDEPENDS_${PN} = "bash"

SRC_URI = "file://config-data/"

APP_INSTALL_DIR = "${base_prefix}/usr/iotc/local"
FILES_${PN} += "${APP_INSTALL_DIR}/*"

# FILES_${PN}-dev = "${APP_INSTALL_DIR}/*"

do_install() {
    install -d ${D}${APP_INSTALL_DIR}/

    cp -R --no-preserve=ownership ${WORKDIR}/config-data/* ${D}${APP_INSTALL_DIR}/
}

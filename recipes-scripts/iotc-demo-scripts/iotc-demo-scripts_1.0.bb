LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

RDEPENDS_${PN} = "bash"

SRC_URI = "file://scripts"

APP_INSTALL_DIR = "${base_prefix}/usr/iotc/local"
FILES_${PN} += "${APP_INSTALL_DIR}*"

do_install() {
    install -d ${D}${APP_INSTALL_DIR}
    # Add command scripts
    for f in ${WORKDIR}/scripts/*
    do
        if [ -f $f ]; then
            if [ ! -d ${D}${APP_INSTALL_DIR}/scripts ]; then
                install -d ${D}${APP_INSTALL_DIR}/scripts
            fi
            install -m 0755 $f ${D}${APP_INSTALL_DIR}/scripts/
        fi
    done

}

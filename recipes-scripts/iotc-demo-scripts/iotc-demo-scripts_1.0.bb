LICENSE = "GPL-3.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-3.0-only;md5=c79ff39f19dfec6d293b95dea7b07891"

RDEPENDS:${PN} = "bash"

SRC_URI = "file://scripts"

APP_INSTALL_DIR = "${base_prefix}/usr/iotc/local"
FILES:${PN} += "${APP_INSTALL_DIR}*"

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

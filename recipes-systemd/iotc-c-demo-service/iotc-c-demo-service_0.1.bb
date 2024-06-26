LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit systemd

SYSTEMD_AUTO_ENABLE = "disable"
SYSTEMD_SERVICE:${PN} = "iotc-c-demo.service"

SRC_URI = "file://iotc-c-demo.service"

FILES:${PN} = "${systemd_unitdir}/system/iotc-c-demo.service"

do_install() {
  install -d ${D}/${systemd_unitdir}/system
  install -m 0644 ${WORKDIR}/iotc-c-demo.service ${D}/${systemd_unitdir}/system
}

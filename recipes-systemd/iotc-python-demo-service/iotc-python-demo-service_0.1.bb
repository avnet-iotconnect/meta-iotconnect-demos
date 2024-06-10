LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit systemd

SYSTEMD_AUTO_ENABLE = "disable"
SYSTEMD_SERVICE_${PN} = "iotc-python-demo.service"

SRC_URI = "file://iotc-python-demo.service"

FILES_${PN} = "${systemd_unitdir}/system/iotc-python-demo.service"

do_install() {
  install -d ${D}/${systemd_unitdir}/system
  install -m 0644 ${WORKDIR}/iotc-python-demo.service ${D}/${systemd_unitdir}/system
}

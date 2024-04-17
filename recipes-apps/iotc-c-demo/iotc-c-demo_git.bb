SUMMARY = "Recipe that compiles and deploys a telemetry demo"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS += " iotc-c-sdk"
RDEPENDS_${PN} += " bash"
RDEPENDS_${PN} += " iotc-c-demo-service"
PROVIDES = "${PN} ${PN}-dev"

SRC_URI = "file://src/"

SRCREV_FORMAT="machine_meta"
SRCREV="${AUTOREV}"

S="${WORKDIR}/src"

inherit cmake

PACKAGES = "${PN} ${PN}-dev ${PN}-dbg ${PN}-staticdev"

APP_INSTALL_DIR = "${base_prefix}/usr/iotc/bin/iotc-c-sdk"
# PRIVATE_DATA_DIR = "${base_prefix}/usr/iotc-c/local"

# FILES_${PN}-dev = "${PRIVATE_DATA_DIR}/* \
# "

FILES_${PN} += "${APP_INSTALL_DIR}/*"

cmake_do_generate_toolchain_file_append() {
	cat >> ${WORKDIR}/toolchain.cmake <<EOF
$cmake_crosscompiling

set( PC_CURL_LIBRARY_DIRS "${STAGING_LIBDIR}")
set( IOTC_C_LIB_INCLUDE_DIR  "${STAGING_BASELIBDIR}/iotc-c-lib/include")
set( IOTC_C_SDK_DIR  "${STAGING_DIR_TARGET}/iotc-generic-c-sdk")
set( IOTC_C_SDK_INCLUDE_DIR  "${STAGING_DIR_TARGET}/iotc-generic-c-sdk/include")
#find_package(CURL REQUIRED)
EOF
}


do_install() {
    install -d ${D}${APP_INSTALL_DIR}
    install -m 0755 iotc-c-demo ${D}${APP_INSTALL_DIR}
    
    # if [ ! -d ${D}${PRIVATE_DATA_DIR} ]; then
    #     install -d ${D}${PRIVATE_DATA_DIR}
    # fi
    # cp -R --no-preserve=ownership ${WORKDIR}/eg-private-repo-data/* ${D}${PRIVATE_DATA_DIR}/

    # # Add command scripts
    # for f in ${WORKDIR}/scripts/*
    # do
    #     if [ -f $f ]; then
    #         if [ ! -d ${D}${APP_INSTALL_DIR}/scripts ]; then
    #             install -d ${D}${APP_INSTALL_DIR}/scripts
    #         fi
    #         install -m 0755 $f ${D}${APP_INSTALL_DIR}/scripts/
    #     fi
    # done
}

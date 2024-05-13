LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "iotc-python-demo-service"
RDEPENDS:${PN} = "python3-iotconnect-sdk bash"

SRC_URI = "gitsm://github.com/avnet-iotconnect/meta-iotconnect-python-demo-source.git;protocol=https;branch=main"
SRCREV="${AUTOREV}"

S="${WORKDIR}/git"

APP_INSTALL_DIR = "${base_prefix}/usr/iotc/bin/iotc-python-sdk"
# PRIVATE_DATA_DIR = "${base_prefix}/usr/iotc-python/local"

# FILES:${PN}-dev = "${PRIVATE_DATA_DIR}/* \
# "

FILES:${PN} +=  "${ROOT_HOME}/ " \
                "${ROOT_HOME}/iotc-application.sh "

FILES:${PN} += "${APP_INSTALL_DIR}/*"

do_install() {
    install -d ${D}${APP_INSTALL_DIR}
    for f in model/*
    do
        if [ -f $f ]; then
            if [ ! -d ${D}${APP_INSTALL_DIR}/model ]; then
                install -d ${D}${APP_INSTALL_DIR}/model
            fi
            install -m 0755 $f ${D}${APP_INSTALL_DIR}/model/
        fi
    done

    # Add command scripts
    # for f in ${WORKDIR}/scripts/*
    # do
    #     if [ -f $f ]; then
    #         if [ ! -d ${D}${APP_INSTALL_DIR}/scripts ]; then
    #             install -d ${D}${APP_INSTALL_DIR}/scripts
    #         fi
    #         install -m 0755 $f ${D}${APP_INSTALL_DIR}/scripts/
    #     fi
    # done

    # Install main app
    install -m 0755 iotc-python-demo.py ${D}${APP_INSTALL_DIR}/

    install -d ${D}/${ROOT_HOME}/
    install -m 0755 iotc-application.sh ${D}${ROOT_HOME}

    # if [ ! -d ${D}${PRIVATE_DATA_DIR} ]; then
    #     install -d ${D}${PRIVATE_DATA_DIR}
    # fi
    # cp -R --no-preserve=ownership ${WORKDIR}/eg-private-repo-data/* ${D}${PRIVATE_DATA_DIR}/

    # Add dummy sensor files
    # echo 1 > ${D}${APP_INSTALL_DIR}/dummy_sensor_power
    # echo 2 > ${D}${APP_INSTALL_DIR}/dummy_sensor_level
}

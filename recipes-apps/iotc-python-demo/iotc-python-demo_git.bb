LICENSE = "GPL-3.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-3.0-only;md5=c79ff39f19dfec6d293b95dea7b07891"

DEPENDS = "iotc-python-demo-service"
RDEPENDS:${PN} = "python3-iotconnect-sdk bash"

SRC_URI = "file://iotc-python-demo.py \
    file://model \
    file://iotc-application.sh \
"

APP_INSTALL_DIR = "${base_prefix}/usr/iotc/bin/iotc-python-sdk"
# PRIVATE_DATA_DIR = "${base_prefix}/usr/iotc-python/local"

# FILES:${PN}-dev = "${PRIVATE_DATA_DIR}/* \
# "

FILES:${PN} +=  "${ROOT_HOME}/ " \
                "${ROOT_HOME}/iotc-application.sh "

FILES:${PN} += "${APP_INSTALL_DIR}/*"

do_install() {
    install -d ${D}${APP_INSTALL_DIR}
    for f in ${WORKDIR}/model/*
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
    install -m 0755 ${WORKDIR}/iotc-python-demo.py ${D}${APP_INSTALL_DIR}/

    install -d ${D}/${ROOT_HOME}/
    install -m 0755 ${WORKDIR}/iotc-application.sh ${D}${ROOT_HOME}

    # if [ ! -d ${D}${PRIVATE_DATA_DIR} ]; then
    #     install -d ${D}${PRIVATE_DATA_DIR}
    # fi
    # cp -R --no-preserve=ownership ${WORKDIR}/eg-private-repo-data/* ${D}${PRIVATE_DATA_DIR}/

    # Add dummy sensor files
    # echo 1 > ${D}${APP_INSTALL_DIR}/dummy_sensor_power
    # echo 2 > ${D}${APP_INSTALL_DIR}/dummy_sensor_level
}

'''
    Converts SDK's dictionary types for readability
'''
from typing import Union # to use Union[str, None] type hint
from iotconnect.IoTConnectSDK import MSGTYPE,ErorCode,CMDTYPE,OPTION
from iotconnect.common.data_evaluation import DATATYPE

class Enums:
    class Keys:
        ack = 'ack'
        command_type = 'ct'
        id = 'id'
        device_command = 'cmd'
        data = 'd'
        data_type = 'dt'

    class Values:
        # 2.1 enums
        class AckStat:
            FAIL = 4
            EXECUTED = 5
            SUCCESS = 7
            EXECUTED_ACK = 6 # what is the difference between this and EXECUTED??
            
        class OtaStat:
            SUCCESS = 0
            FAILED = 1
            DL_IN_PROGRESS = 2
            DL_DONE = 3
            DL_FAILED = 4

        class MessageType:
            RPT = MSGTYPE["RPT"]
            FLT = MSGTYPE["FLT"]
            RPTEDGE = MSGTYPE["RPTEDGE"]
            RMEdge = MSGTYPE["RMEdge"]
            LOG = MSGTYPE["LOG"]
            ACK = MSGTYPE["ACK"]
            OTA = MSGTYPE["OTA"]
            FIRMWARE = MSGTYPE["FIRMWARE"]
            REQUEST_ATTRIBUTES = 201

        class ErrorCode:
            OK = ErorCode["OK"]
            DEV_NOT_REG = ErorCode["DEV_NOT_REG"]
            AUTO_REG = ErorCode["AUTO_REG"]
            DEV_NOT_FOUND = ErorCode["DEV_NOT_FOUND"]
            DEV_INACTIVE = ErorCode["DEV_INACTIVE"]
            OBJ_MOVED = ErorCode["OBJ_MOVED"]
            CPID_NOT_FOUND = ErorCode["CPID_NOT_FOUND"]

        class Commands:
            DEVICE_COMMAND = CMDTYPE["DCOMM"]
            FIRMWARE = CMDTYPE["FIRMWARE"]
            MODULE = CMDTYPE["MODULE"]
            U_ATTRIBUTE = CMDTYPE["U_ATTRIBUTE"]
            U_SETTING = CMDTYPE["U_SETTING"]
            U_RULE = CMDTYPE["U_RULE"]
            U_DEVICE = CMDTYPE["U_DEVICE"]
            DATA_FRQ = CMDTYPE["DATA_FRQ"]
            U_barred = CMDTYPE["U_barred"]
            D_Disabled = CMDTYPE["D_Disabled"]
            D_Released = CMDTYPE["D_Released"]
            STOP = CMDTYPE["STOP"]
            Start_Hr_beat = CMDTYPE["Start_Hr_beat"]
            Stop_Hr_beat = CMDTYPE["Stop_Hr_beat"]
            INIT_CONNECT = CMDTYPE["is_connect"]
            SYNC = CMDTYPE["SYNC"]
            RESETPWD = CMDTYPE["RESETPWD"]
            UCART = CMDTYPE["UCART"]

        class Option:
            attribute = OPTION["attribute"]
            setting = OPTION["setting"]
            protocol = OPTION["protocol"]
            device = OPTION["device"]
            sdkConfig = OPTION["sdkConfig"]
            rule = OPTION["rule"]

    class MetadataKeys:
        name = 'ln'
        data_type = 'dt'
        default_value = 'dv'
        sequence = 'sq'

    class ReadTypes:
        ascii = "ascii"
        binary = "binary"

    class SendDataTypes:
        INT = DATATYPE["INT"]
        LONG = DATATYPE["LONG"]
        FLOAT = DATATYPE["FLOAT"]
        STRING = DATATYPE["STRING"]
        Time = DATATYPE["Time"]
        Date = DATATYPE["Date"]
        DateTime = DATATYPE["DateTime"]
        BIT = DATATYPE["BIT"]
        Boolean = DATATYPE["Boolean"]
        LatLong = DATATYPE["LatLong"]
        OBJECT = DATATYPE["OBJECT"]


    @classmethod
    def get_value(cls,msg, key) -> Union[str, None]:
        if (key in msg):
            return msg[key]
        return None

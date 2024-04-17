import json
from datetime import datetime

from iotconnect import IoTConnectSDK

from model.enums import Enums as E


def print_msg(title, msg):
    print("{}: \n{}".format(title, json.dumps(msg, indent=2)))


class GenericDevice:
    template = None
    children = None
    """
    minimal device, no connectivity, has to be child device
    """
    def __init__(self, unique_id, tag=None):
        self.unique_id = unique_id
        self.name = unique_id
        self.tag = tag

    def for_iotconnect_upload(self):
        export_dict = {
            "name": self.name,
            "uniqueId": self.unique_id,
            "tag": self.tag,
            "properties": []
        }
        return export_dict

    def get_d2c_data(self):
        data_obj = [{
            "uniqueId": self.unique_id,
            "time": datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.000Z"),
            "data": self.get_state()
        }]
        return data_obj
    
        
    def generate_d2c_data(self, data):
        data_obj = [{
            "uniqueId": self.unique_id,
            "time": datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.000Z"),
            "data": data
        }]
        return data_obj

    def get_state(self) -> dict:
        raise NotImplementedError()


class ConnectedDevice(GenericDevice):
    needs_exit:bool = False
    in_ota:bool = False
    attribute_metadata: list = None

    def __init__(self, company_id, unique_id, environment, sdk_id, sdk_options=None):
        super().__init__(unique_id)
        self.company_id = company_id
        self.environment = environment
        self.sdk_id = sdk_id
        self.SdkClient = None
        self.SdkOptions = sdk_options

    def connect(self):
        self.SdkClient = IoTConnectSDK(
            uniqueId=self.unique_id,
            # sId=self.sdk_id,
            # cpid=self.company_id,
            #env=self.environment,
            sdkOptions=self.SdkOptions,
            initCallback=self.init_cb)
        
        self.bind_callbacks()
        self.SdkClient.GetAttributes(self.get_attribute_metadata_from_cloud)

    def get_attribute_metadata_from_cloud(self, msg):
        self.attribute_metadata = []
        for meta_dict in msg:
            if E.Keys.data in meta_dict:
                self.attribute_metadata = meta_dict[E.Keys.data]


    def ota_cb(self,msg):
        raise NotImplementedError()

    def module_cb(self,msg):
        raise NotImplementedError()

    def twin_change_cb(self,msg):
        raise NotImplementedError()

    def attribute_change_cb(self,msg):
        self.SdkClient.GetAttributes(self.get_attribute_metadata_from_cloud)

    def device_change_cb(self,msg):
        raise NotImplementedError()

    def rule_change_cb(self,msg):
        raise NotImplementedError()

    def device_cb(self, msg):
        raise NotImplementedError()

    def init_cb(self, msg):
        if E.get_value(msg, E.Keys.command_type) is E.Values.Commands.INIT_CONNECT:
            print("connection status is " + msg["command"])
        
    def bind_callbacks(self):
        self.SdkClient.onOTACommand(self.ota_cb)
        self.SdkClient.onModuleCommand(self.module_cb)
        self.SdkClient.onTwinChangeCommand(self.twin_change_cb)
        self.SdkClient.onAttrChangeCommand(self.attribute_change_cb)
        self.SdkClient.onDeviceChangeCommand(self.device_change_cb)
        self.SdkClient.onRuleChangeCommand(self.rule_change_cb)
        self.SdkClient.onDeviceCommand(self.device_cb)

    def send_device_states(self):

        # Don't send anything till we get our cloud attributes
        if self.attribute_metadata is None:
            return

        data_array = [self.get_d2c_data()]
        if self.children is not None:
            for child in self.children:
                data_array.append(child.get_d2c_data())

        for data in data_array:
            self.send_d2c(data)
        return data_array

    def send_d2c(self, data):
        if self.SdkClient is not None:
            self.SdkClient.SendData(data)
        else:
            print("no client")

    def send_ack(self, msg, status: E.Values.AckStat, message):
        # check if ack exists in message
        if E.get_value(msg, E.Keys.ack) is None:
            print("Ack not requested, returning")
            return
        
        id_to_send = E.get_value(msg, E.Keys.id)
        self.SdkClient.sendAckCmd(msg[E.Keys.ack], status, message, id_to_send)


class Gateway(ConnectedDevice):
    children = []

    def show_children(self):
        if self.children.count:
            print("children")
            for child in self.children:
                print(child.unique_id)
        else:
            print("no children")

    def for_iotconnect_upload(self):
        export_dict = {
            "gateway": {
                "items": []
            }
        }
        for child in self.children:
            export_dict["gateway"]["items"].append(child.for_iotconnect_upload())
        return export_dict

from app import app
from app import db

import datetime
import hashlib
import binascii
import json
import requests

from flask import Flask, jsonify, abort, request
from flask import render_template, flash, redirect

class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(20))

    def __init__(self, name):
        self.name = name

    def __repr__(self):
        return '<User %r>' % (self.nickname)


class Post(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    body = db.Column(db.String(140))
    timestamp = db.Column(db.DateTime)
    # user_id = db.Column(db.Integer, db.ForeignKey('user.id'))

    def __repr__(self):
        return '<Post %r>' % (self.body)

def load_data(data_obj):
    
    for table_name in data_obj.keys():
        data_obj_list = data_obj[table_name]
        #print("table_name=" + table_name,  flush=True)
        #print("data_obj_list= " + str(data_obj_list))

    return data_obj_list,table_name

def table_assign(data_obj_list,table_name,operation):
    
    print("select table_name=" + str(table_name),  flush=True)
    if table_name=="MainHost":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = MainHost(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = MainHost.query.filter_by(customer_number=customer_number).first()
    elif table_name=="CustomerIP":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = CustomerIP(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = CustomerIP.query.filter_by(customer_number=customer_number).first()
    elif table_name=="Preservation_host":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = Preservation_host(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = Preservation_host.query.filter_by(customer_number=customer_number).first()
    elif table_name=="KeepAlive":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = KeepAlive(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = KeepAlive.query.filter_by(customer_number=customer_number).first()
    elif table_name=="KeepAliveDevice":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = KeepAliveDevice(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = KeepAliveDevice.query.filter_by(customer_number=customer_number).first()
    elif table_name=="Partition":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = Partition(**data_obj_list)
        elif operation=="delete" or operation=="update":
            partition_ID=data_obj_list["partition_ID"]
            u = Partition.query.filter_by(partition_ID=partition_ID).first()
    elif table_name=="PartitionLoop":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = PartitionLoop(**data_obj_list)
        elif operation=="delete" or operation=="update":
            partition_ID=data_obj_list["partition_ID"]
            u = PartitionLoop.query.filter_by(partition_ID=partition_ID).first()
    elif table_name=="Loop":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = Loop(**data_obj_list)
        elif operation=="delete" or operation=="update":
            loop_ID=data_obj_list["loop_ID"]
            u = Loop.query.filter_by(loop_ID=loop_ID).first()
    elif table_name=="LoopDevice":
        u = LoopDevice(**data_obj_list)
    elif table_name=="Device":
        u = Device(**data_obj_list)
    elif table_name=="DeviceType":
        if operation=="add":
            u = DeviceType(**data_obj_list)
        elif operation=="delete" or operation=="update":
            type_ID=data_obj_list["type_ID"]
            u = DeviceType.query.filter_by(type_ID=type_ID).first()
    elif table_name=="CouplingLoop":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = CouplingLoop(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = CouplingLoop.query.filter_by(customer_number=customer_number).first()
    elif table_name=="DeviceSetting":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = DeviceSetting(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = DeviceSetting.query.filter_by(customer_number=customer_number).first()
    elif table_name=="WLReadSensor":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = WLReadSensor(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = WLReadSensor.query.filter_by(customer_number=customer_number).first()
    elif table_name=="WLRemoteControl":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = WLRemoteControl(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = WLRemoteControl.query.filter_by(customer_number=customer_number).first()
    elif table_name=="WRS_WRC":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = WRS_WRC(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = WRS_WRC.query.filter_by(customer_number=customer_number).first()
    elif table_name=="WLDoubleBondBTemperatureSensor":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = WLDoubleBondBTemperatureSensor(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = WLDoubleBondBTemperatureSensor.query.filter_by(customer_number=customer_number).first()
    elif table_name=="WLReedSensor":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = WLReedSensor(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = WLReedSensor.query.filter_by(customer_number=customer_number).first()
    elif table_name=="WLCamera":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = WLCamera(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = WLCamera.query.filter_by(customer_number=customer_number).first()
    elif table_name=="Signal":
        if operation=="add":
            data_obj_list["time"]=datetime.datetime.now()
            u = Signal(**data_obj_list)
        elif operation=="delete" or operation=="update":
            customer_number=data_obj_list["customer_number"]
            u = Signal.query.filter_by(customer_number=customer_number).first()
    elif table_name=="Signal_Event":
        u = Signal_Event(**data_obj_list)
    else:
        return jsonify('No such table in DB, return error!!!'), 501

    return u

def database_add(data_obj):

    print("data_obj to add:" + str(data_obj))
    data_obj_list,table_name = load_data(data_obj)
    operation="add"
    addunit = table_assign(data_obj_list,table_name,operation)
    db.session.add(addunit)
    db.session.commit()

    print('Add to DB success!!!')
    return 

def database_update(data_obj):

    data_obj_list,table_name = load_data(data_obj)
    operation="update"
    unit = table_assign(data_obj_list,table_name,operation)

    if unit:
        db.session.delete(unit)
        db.session.commit()
    
    database_add(data_obj)

    print('Update to DB success!!!')
    return 

def database_delete(data_obj):

    data_obj_list,table_name = load_data(data_obj)
    operation="delete"
    unit = table_assign(data_obj_list,table_name,operation)

    if unit:
        print('Delete from DB success!!!')
    else:
        print('No data found in DB, return error!!!')
        return False
    
    db.session.delete(unit)
    db.session.commit()
    return True

################################################################
class MainHost(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    mac_identify = db.Column(db.String(32))
    customer_number = db.Column(db.String(32), index = True)


class CustomerIP(db.Model):
    id = db.Column(db.Integer, primary_key=True, index = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    ip = db.Column(db.String(16))


class Preservation_host(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    partial_transmission_set = db.Column(db.String(4))
    urgent_bt_alarm_sound_set = db.Column(db.String(4))
    alarm_action_time_set = db.Column(db.Integer)
    alarm_contact_output_action_time = db.Column(db.Integer)
    alarm_contact_output_action_status = db.Column(db.String(8))
    alarm_contact_output_set = db.Column(db.String(4))
    v_output_contact_action_time = db.Column(db.Integer)
    v_output_contact_action_status = db.Column(db.String(8))
    v_contact_output_set = db.Column(db.String(4))
    network_set = db.Column(db.String(16))
    network_attribute = db.Column(db.String(80))
    action_temperature_sensing = db.Column(db.String(8))
    action_temperature_sensing_gap = db.Column(db.String(8))
    connection_mac = db.Column(db.String(24))
    battery_low_power_set = db.Column(db.String(8))
    battery_low_power_gap = db.Column(db.String(8))


class KeepAlive(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    now_connection_way = db.Column(db.String(20))
    now_connection_status = db.Column(db.String(8))
    support_connection_way = db.Column(db.String(10))
    foolproof_status = db.Column(db.String(4))
    power_source = db.Column(db.String(10))
    low_power_control = db.Column(db.String(8))
    power_status = db.Column(db.String(8))
    temperature_control = db.Column(db.String(8))
    temperature_status = db.Column(db.String(8))
    control_way = db.Column(db.String(4))
    json_version = db.Column(db.String(30))
    preservation_set_time = db.Column(db.String(10))
    preservation_lift_time = db.Column(db.String(10))
    preservation_delay_time = db.Column(db.Integer)
    standard_delay_set_time = db.Column(db.Integer)
    preservation_set_time_delay_set = db.Column(db.DateTime)
    preservation_set_time_change = db.Column(db.DateTime)
    preservation_lift_time_change = db.Column(db.DateTime)
    holiday = db.Column(db.DateTime)
    trigger_behindtime_set_unusual_record = db.Column(db.String(4))
    trigger_early_lift_unusual_record = db.Column(db.String(4))
    start_host_set = db.Column(db.String(4))
    start_send_keepalive = db.Column(db.String(4))
    admin_password = db.Column(db.String(30))
    service_push_notification_status = db.Column(db.String(4))
    customer_push_notification_status = db.Column(db.String(4))
    customer_unusual_image_send = db.Column(db.String(4))
    customer_status = db.Column(db.String(4))
    send_message_major_url = db.Column(db.String(80))
    send_message_minor_url = db.Column(db.String(80))

#5
class KeepAliveDevice(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32),index = True)
    loop_ID = db.Column(db.String(40))
    type_ID = db.Column(db.String(10))
    device_ID = db.Column(db.String(40))
    temperature_set = db.Column(db.String(8))
    power_set = db.Column(db.String(8))
    connection_status = db.Column(db.String(8))
    signal_power_status = db.Column(db.String(8))
    temperature_status = db.Column(db.String(8))
    power_status = db.Column(db.String(8))
    action = db.Column(db.String(10))
    json_version = db.Column(db.String(32))


class Partition(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32))
    partition_ID = db.Column(db.String(12), index = True)


class PartitionLoop(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32))
    partition_ID = db.Column(db.String(12), index = True)
    loop_ID = db.Column(db.String(40))


class Loop(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32))
    loop_ID = db.Column(db.String(40), index = True)
    status = db.Column(db.String(4))


class LoopDevice(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32))
    loop_ID = db.Column(db.String(40))
    device_ID = db.Column(db.String(40))

#10
class Device(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32))
    device_ID = db.Column(db.String(40))
    identify = db.Column(db.String(4))
    name = db.Column(db.String(40))
    type_ID = db.Column(db.String(10))


class DeviceType(db.Model):
    type_ID = db.Column(db.String(10), primary_key = True, index = True)
    type_name_tn = db.Column(db.String(50))
    type_name_en = db.Column(db.String(50))


class CouplingLoop(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    opticalcoupling = db.Column(db.String(40))
    loop_ID = db.Column(db.String(40))
    status = db.Column(db.String(8))


class DeviceSetting(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    type_ID = db.Column(db.String(10))
    sendMsg_time_on_set = db.Column(db.Integer)
    sendMsg_time_off_set = db.Column(db.Integer)


class WLReadSensor(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    device_ID = db.Column(db.String(40), index = True)
    G_sensor_status = db.Column(db.String(4))
    G_sensitivity = db.Column(db.String(10))
    power_lock_status = db.Column(db.String(4))
    power_lock_action_time = db.Column(db.Integer)
    msg_send_strong_set = db.Column(db.String(10))
    msg_send_strong_set_manual = db.Column(db.String(10))
    msg_send_strong_set_manual_gap = db.Column(db.String(10))
    temperature_sensing = db.Column(db.String(10))
    temperature_sensing_gap = db.Column(db.String(10))
    terminating_impedance = db.Column(db.String(10))
    battery_low_power_set = db.Column(db.String(10))
    battery_low_power_gap = db.Column(db.String(10))
    transmission_distance = db.Column(db.String(10))
    transmission_distance_set_manual = db.Column(db.Integer)

#15
class WLRemoteControl(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    device_ID = db.Column(db.String(40), index = True)
    patterns = db.Column(db.String(10))
    partition_ID = db.Column(db.String(40))


class WRS_WRC(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    WRS_ID = db.Column(db.String(40))
    WRC_ID = db.Column(db.String(40))


class WLDoubleBondBTemperatureSensor(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    device_ID = db.Column(db.String(40))
    G_sensor_status = db.Column(db.String(4))
    G_sensitivity = db.Column(db.String(10))
    msg_send_strong_set = db.Column(db.String(10))
    msg_send_strong_set_manual = db.Column(db.String(10))
    msg_send_strong_set_manual_gap = db.Column(db.String(10))
    temperature_sensing = db.Column(db.String(10))
    temperature_sensing_gap = db.Column(db.String(10))
    connection_time_set = db.Column(db.String(8))
    connection_time_set_seconds_manual = db.Column(db.Integer)
    action_count_frequency = db.Column(db.Integer)
    action_count_time = db.Column(db.Integer)
    microwave_set = db.Column(db.String(4))
    microwave_sensitivity_set = db.Column(db.String(10))
    infrared_set = db.Column(db.String(4))
    terminating_impedance = db.Column(db.String(10))
    battery_low_power_set = db.Column(db.String(10))
    battery_low_power_gap = db.Column(db.String(10))
    transmission_distance = db.Column(db.String(10))
    transmission_distance_set_manual = db.Column(db.Integer)


class WLReedSensor(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    device_ID = db.Column(db.String(40))
    G_sensor_status = db.Column(db.String(4))
    G_sensitivity = db.Column(db.String(10))
    msg_send_strong_set = db.Column(db.String(10))
    msg_send_strong_set_manual = db.Column(db.String(10))
    msg_send_strong_set_manual_gap = db.Column(db.String(10))
    temperature_sensing = db.Column(db.String(10))
    temperature_sensing_gap = db.Column(db.String(10))
    connection_time_set = db.Column(db.String(8))
    connection_time_set_seconds_manual = db.Column(db.Integer)
    action_count_frequency = db.Column(db.Integer)
    action_count_time = db.Column(db.Integer)
    microwave_set = db.Column(db.String(4))
    microwave_sensitivity_set = db.Column(db.String(10))
    infrared_set = db.Column(db.String(4))
    terminating_impedance = db.Column(db.String(10))
    battery_low_power_set = db.Column(db.String(10))
    battery_low_power_gap = db.Column(db.String(10))
    transmission_distance = db.Column(db.String(10))
    transmission_distance_set_manual = db.Column(db.Integer)


class WLCamera(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    device_ID = db.Column(db.String(40))
    frame = db.Column(db.Integer)
    pixel = db.Column(db.Integer)
    e_before_seconds = db.Column(db.Integer)
    e_after_seconds = db.Column(db.Integer)
    real_time_moniter = db.Column(db.String(4))
    media_upload_set = db.Column(db.String(4))
    PIR_set = db.Column(db.String(4))
    battery_low_power_set = db.Column(db.String(10))
    battery_low_power_gap = db.Column(db.String(10))

#20
class Signal(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32), index = True)
    group_ID = db.Column(db.String(24))
    loop_ID = db.Column(db.String(12))
    type_ID = db.Column(db.String(10))
    device_ID = db.Column(db.String(32))
    setting_value = db.Column(db.String(12))
    abnormal_value = db.Column(db.String(12))
    alert_catalog = db.Column(db.String(10))
    abnormal_catalog = db.Column(db.String(10))
    situation_descript = db.Column(db.String(50))
    abnormal_descript = db.Column(db.String(50))
    media_file_path = db.Column(db.String(100))
    send_customer_status = db.Column(db.String(4))


class Signal_Event(db.Model):
    id = db.Column(db.Integer, primary_key = True, unique = True)
    time = db.Column(db.DateTime)
    reg_time = db.Column(db.DateTime)
    customer_number = db.Column(db.String(32))
    group_ID = db.Column(db.String(24), index = True, unique = True)
    alert_catalog = db.Column(db.String(12))
    handler_status = db.Column(db.String(4))
    checker = db.Column(db.String(4))
    validate_code = db.Column(db.String(24))
    send_service_status = db.Column(db.String(4))

#!/usr/bin/python
import time
import json
import spidev
import requests

# This program reads an analog value form a light sensor attached to port 0 on the MCP3008 Chip

spi = spidev.SpiDev()
adc_port = 0
threshold0 = 10
threshold1 = 250

variance = 5

d1 = 50
d2 = 100
d3 = 150

host = "https://png.d2d.msg.intuit.com/api/v2/push"
senderId = "********"
dry_run = False
message = """{"style": "BigTextStyle", "contentTitle": "Lumen Raspi", "contentText": "$COND$","BigTextStyle": {"bigContentTitle": "Lumen Raspi $COND$","bigText": "Lighting conditions changed from $K0$ to $K1$","summary": "Lighting changed from $K0$ to $K1$"},"smallIcon": "ic_lightbulb","background": "ic_lightbulb_$ICON$"}"""


def analog_read(port, bus=0, ce=0):
    """Read the given ADC port and preform the necessary shifting of bits"""
    spi.open(bus, ce)  # CE port that the MCP3008 is connected to
    if (port > 7) or (port < 0):
        print 'analog_read -- Port Error, Must use a port between 0 and 7'
        return -1
    r = spi.xfer2([1, (8 + port) << 4, 0])
    value = ((r[1] & 3) << 8) + r[2]
    spi.close()
    return value


def encode_msg(grp, k1, k0, cond, icon):
    """Create a JSON message structure to be processed at PNG, enclosed payload is for the mobile/wear target

    :param grp: receiver group or groups
    :param k1: most current sensor reading
    :param k0: previous sensor reading
    :param cond: current lighting condition
    :param icon: icon-identifier
    :return:json encoded payload inside envelope
    """
    data = dict()
    data['payload'] = message.replace("$K1$", str(k1)).replace("$K0$", str(k0)).replace("$COND$", cond).replace(
        "$ICON$",
        icon)

    gcm = dict()
    gcm['dry_run'] = dry_run
    gcm['time_to_live'] = 1
    gcm['groups'] = grp
    gcm['data'] = data

    msg = dict()
    msg['senderId'] = senderId
    msg['gcm'] = gcm
    return json.dumps(msg)


def interpret_data(k0, k1):
    """Determine if lighting conditions have changed since the last reading

    :param k0: previous reading
    :param k1: current sensor reading
    :return: json encoded message, to be consumed by an Intuit Wear SDK client
    """
    icon = "b"
    cond = ""
    groups = list()

    if d >= d1:
        groups.append('d1')
        cond = "Small Fluctuation"
        icon = "r"
    if d >= d2:
        groups.append('d2')
        cond = "Medium Fluctuation"
    if d >= d3:
        groups.append('d3')
        cond = "Large Fluctuation"

    if k0 < threshold1 < k1:
        # natural to artificial
        groups.append('artificial')
        icon = "y"
        cond = "Artificial Lighting"
    elif k0 > threshold1 > k1 :
        # artificial to natural
        groups.append('natural')
        icon = "g"
        cond = "Natural Lighting"
    elif k0 < threshold0 < k1:
        # dark to natural or artificial
        if k1 < threshold1:
            groups.append('natural')
            icon = "g"
            cond = "Natural Lighting"
        else:
            groups.append('artificial')
            icon = "y"
            cond = "Artificial Lighting"
    elif k0 > threshold0 > k1 :
        # natural or artificial to darkness
        groups.append('dark')
        cond = "Absence of Light"

    if 0 < len(groups):
        print cond
        return encode_msg(groups, k1, k0, cond, icon)
    else:
        return None


k0 = analog_read(adc_port) - threshold0
while True:
    time.sleep(1)  # Wait a second
    k1 = analog_read(adc_port)
    d = abs(k1 - k0)
    print k1, d
    if (d > variance):
        msg = interpret_data(k0, k1)
        k0 = k1
        if (msg):
            try:
                r = requests.post(host, headers={'Content-Type': 'application/json'}, data=msg)
                print r.status_code
            except:
                pass



Receives Ethernet II (DIX) frames with an optional 802.1q vlan tag, checks the destination MAC address and sends frames to corresponding port(s). It has a data buffer for each port and keeps track of connected devices to each port in a (MAC address, vlan) table.

A client app creates a TAP (a virtual-network link-layer kernel device) which forwards all the outgoing traffic from interface to switch, and from switch back to interface.

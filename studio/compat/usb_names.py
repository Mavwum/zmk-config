#!/usr/bin/env python3
# Replace only this Studio build's product-string request; the upstream source,
# VID/PID, interfaces, serial number and other USB descriptors are unchanged.
from pathlib import Path
import sys
source=Path(sys.argv[1]).read_text()
needle='\tLOG_DBG("Get Descriptor request");'
assert source.count(needle)==1,'Pinned USB descriptor handler changed'
hook='\textern const uint8_t *keebart_usb_name(void);\n\tif (USB_GET_DESCRIPTOR_TYPE(setup->wValue) == USB_DESC_STRING &&\n\t    USB_GET_DESCRIPTOR_INDEX(setup->wValue) == 2) {\n\t\tconst uint8_t *name = keebart_usb_name();\n\t\tif (name) { *len = name[0]; *data = (uint8_t *)name; return true; }\n\t}\n'
Path(sys.argv[2]).write_text(source.replace(needle,hook+needle))

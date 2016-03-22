/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2001-2002  Nokia Corporation
 *  Copyright (C) 2002-2003  Maxim Krasnyansky <maxk@qualcomm.com>
 *  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2002-2003  Stephen Crane <steve.crane@rococosoft.com>
 *  Copyright (C) 2002-2003  Jean Tourrilhes <jt@hpl.hp.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <signal.h>
#include <unistd.h>

//signal handler to break out of main loop
static int running = 1;
void sig_handler(int sig)
{
  running = 0;
}

int register_service_wiimote0(sdp_session_t * session)
{
	sdp_record_t record;
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, hid_uuid, l2cap_uuid, hidp_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[3];
	sdp_data_t *psm, *lang_lst, *lang_lst2, *hid_spec_lst, *hid_spec_lst2;
	unsigned int i;
	uint8_t dtd = SDP_UINT16;
	uint8_t dtd2 = SDP_UINT8;
	uint8_t dtd_data = SDP_TEXT_STR8;
	void *dtds[2];
	void *values[2];
	void *dtds2[2];
	void *values2[2];
	int leng[2];
	uint8_t hid_spec_type = 0x22;
	uint16_t hid_attr_lang[] = { 0x409, 0x100 };
	uint16_t ctrl = 0x11, intr = 0x13;
	uint16_t hid_release = 0x0100, parser_version = 0x0111;
	uint8_t subclass = 0x04, country = 0x33;
	uint8_t virtual_cable = 0, reconnect = 1, sdp_disable = 0;
	uint8_t battery = 1, remote_wakeup = 1;
	uint16_t profile_version = 0x0100, superv_timeout = 0x0c80;
	uint8_t norm_connect = 0, boot_device = 0;
	const uint8_t hid_spec[] = {
		0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x10,
		0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95,
		0x01, 0x06, 0x00, 0xff, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x11, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x12, 0x95, 0x02, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x13, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x14, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x15, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x16, 0x95, 0x15, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x17, 0x95, 0x06, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x18, 0x95, 0x15, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x19, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x1a, 0x95, 0x01, 0x09, 0x01, 0x91, 0x00,
		0x85, 0x20, 0x95, 0x06, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x21, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x22, 0x95, 0x04, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x30, 0x95, 0x02, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x31, 0x95, 0x05, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x32, 0x95, 0x0a, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x33, 0x95, 0x11, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x34, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x35, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x36, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x37, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x3d, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x3e, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0x85, 0x3f, 0x95, 0x15, 0x09, 0x01, 0x81, 0x00,
		0xc0, 0x00
	};

	memset(&record, 0, sizeof(sdp_record_t));
	record.handle = 0xffffffff;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(&record, root);

	sdp_uuid16_create(&hid_uuid, HID_SVCLASS_ID);
	svclass_id = sdp_list_append(NULL, &hid_uuid);
	sdp_set_service_classes(&record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, HID_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(NULL, profile);
	sdp_set_profile_descs(&record, pfseq);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[1] = sdp_list_append(0, &l2cap_uuid);
	psm = sdp_data_alloc(SDP_UINT16, &ctrl);
	proto[1] = sdp_list_append(proto[1], psm);
	apseq = sdp_list_append(0, proto[1]);

	sdp_uuid16_create(&hidp_uuid, HIDP_UUID);
	proto[2] = sdp_list_append(0, &hidp_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(&record, aproto);

	proto[1] = sdp_list_append(0, &l2cap_uuid);
	psm = sdp_data_alloc(SDP_UINT16, &intr);
	proto[1] = sdp_list_append(proto[1], psm);
	apseq = sdp_list_append(0, proto[1]);

	sdp_uuid16_create(&hidp_uuid, HIDP_UUID);
	proto[2] = sdp_list_append(0, &hidp_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_add_access_protos(&record, aproto);

	sdp_add_lang_attr(&record);

	sdp_set_info_attr(&record, "Nintendo RVL-CNT-01",
					"Nintendo", "Nintendo RVL-CNT-01");

	sdp_attr_add_new(&record, SDP_ATTR_HID_DEVICE_RELEASE_NUMBER,
						SDP_UINT16, &hid_release);

	sdp_attr_add_new(&record, SDP_ATTR_HID_PARSER_VERSION,
						SDP_UINT16, &parser_version);

	sdp_attr_add_new(&record, SDP_ATTR_HID_DEVICE_SUBCLASS,
						SDP_UINT8, &subclass);

	sdp_attr_add_new(&record, SDP_ATTR_HID_COUNTRY_CODE,
						SDP_UINT8, &country);

	sdp_attr_add_new(&record, SDP_ATTR_HID_VIRTUAL_CABLE,
						SDP_BOOL, &virtual_cable);

	sdp_attr_add_new(&record, SDP_ATTR_HID_RECONNECT_INITIATE,
						SDP_BOOL, &reconnect);

	dtds[0] = &dtd2;
	values[0] = &hid_spec_type;
	dtds[1] = &dtd_data;
	values[1] = (uint8_t *) hid_spec;
	leng[0] = 0;
	leng[1] = sizeof(hid_spec)-1;
	hid_spec_lst = sdp_seq_alloc_with_length(dtds, values, leng, 2);
	hid_spec_lst2 = sdp_data_alloc(SDP_SEQ8, hid_spec_lst);
	sdp_attr_add(&record, SDP_ATTR_HID_DESCRIPTOR_LIST, hid_spec_lst2);

	for (i = 0; i < sizeof(hid_attr_lang) / 2; i++) {
		dtds2[i] = &dtd;
		values2[i] = &hid_attr_lang[i];
	}

	lang_lst = sdp_seq_alloc(dtds2, values2, sizeof(hid_attr_lang) / 2);
	lang_lst2 = sdp_data_alloc(SDP_SEQ8, lang_lst);
	sdp_attr_add(&record, SDP_ATTR_HID_LANG_ID_BASE_LIST, lang_lst2);

	sdp_attr_add_new(&record, SDP_ATTR_HID_SDP_DISABLE,
						SDP_BOOL, &sdp_disable);

	sdp_attr_add_new(&record, SDP_ATTR_HID_BATTERY_POWER,
						SDP_BOOL, &battery);

	sdp_attr_add_new(&record, SDP_ATTR_HID_REMOTE_WAKEUP,
						SDP_BOOL, &remote_wakeup);

	sdp_attr_add_new(&record, SDP_ATTR_HID_PROFILE_VERSION,
						SDP_UINT16, &profile_version);

	sdp_attr_add_new(&record, SDP_ATTR_HID_SUPERVISION_TIMEOUT,
						SDP_UINT16, &superv_timeout);

	sdp_attr_add_new(&record, SDP_ATTR_HID_NORMALLY_CONNECTABLE,
						SDP_BOOL, &norm_connect);

	sdp_attr_add_new(&record, SDP_ATTR_HID_BOOT_DEVICE,
						SDP_BOOL, &boot_device);

	return sdp_record_register(session, &record, 0);
  //SDP_RECORD_PERSIST
}

int register_service_wiimote1(sdp_session_t * session)
{
    uint16_t l2cap_channel = 1;
    sdp_profile_desc_t profile;
    uint16_t spec_id = 0x0100, vend_id = 0x057e, prod_id = 0x306;
    uint16_t version = 0x0600, id_src = 0x0002;
    uint8_t primary_rec = 1;

    uuid_t root_uuid, l2cap_uuid, sdp_uuid, svc_uuid, pnp_uuid;
    sdp_list_t *l2cap_list = 0,
               *sdp_list = 0,
               *root_list = 0,
               *proto_list = 0,
               *access_proto_list = 0,
               *profile_list = 0,
               *class_list = 0;
    sdp_data_t *channel = 0, *psm = 0;

    sdp_record_t *record = sdp_record_alloc();

    // service class id list
    sdp_uuid16_create(&pnp_uuid, PNP_INFO_SVCLASS_ID);
    class_list = sdp_list_append(NULL, &pnp_uuid);
    sdp_set_service_classes(record, class_list);

    // set l2cap information
    sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
    channel = sdp_data_alloc(SDP_UINT16, &l2cap_channel);
    l2cap_list = sdp_list_append( 0, &l2cap_uuid );
    sdp_list_append( l2cap_list, channel );
    proto_list = sdp_list_append( 0, l2cap_list );

    // set sdp information
    sdp_uuid16_create(&sdp_uuid, SDP_UUID);
    sdp_list = sdp_list_append( 0, &sdp_uuid );
    sdp_list_append( proto_list, sdp_list );

    // attach protocol information to service record
    access_proto_list = sdp_list_append( 0, proto_list );
    sdp_set_access_protos( record, access_proto_list );

    // make the service record publicly browsable
    sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
    root_list = sdp_list_append(0, &root_uuid);
    sdp_set_browse_groups( record, root_list );

    // profile descriptor list
    sdp_uuid16_create(&profile.uuid, PNP_INFO_PROFILE_ID);
    profile.version = 0x0100;
    profile_list = sdp_list_append(0, &profile);
    sdp_set_profile_descs(record, profile_list);

    //attributes
    sdp_attr_add_new(record, SDP_ATTR_SPECIFICATION_ID, SDP_UINT16, &spec_id);
    sdp_attr_add_new(record, SDP_ATTR_VENDOR_ID, SDP_UINT16, &vend_id);
    sdp_attr_add_new(record, SDP_ATTR_PRODUCT_ID, SDP_UINT16, &prod_id);
    sdp_attr_add_new(record, SDP_ATTR_VERSION, SDP_UINT16, &version);
    sdp_attr_add_new(record, SDP_ATTR_PRIMARY_RECORD, SDP_BOOL, &primary_rec);
    sdp_attr_add_new(record, SDP_ATTR_VENDOR_ID_SOURCE, SDP_UINT16, &id_src);

    int err = 0;

    // connect to the local SDP server, register the service record, and
    // disconnect
    err = sdp_record_register(session, record, 0);

    // cleanup
    sdp_data_free( channel );
    sdp_list_free( l2cap_list, 0 );
    sdp_list_free( sdp_list, 0 );
    sdp_list_free( root_list, 0 );
    sdp_list_free( access_proto_list, 0 );

    return err;
}

int main()
{
  sdp_session_t * session;
  session = sdp_connect( BDADDR_ANY, BDADDR_LOCAL, SDP_RETRY_IF_BUSY );

  signal(SIGINT, sig_handler);

  register_service_wiimote0(session);
  register_service_wiimote1(session);

  while(running)
  {
    usleep(1000);
  }

  sdp_close(session);
}


// *****************************************************************************
//
// test battery service
//
// *****************************************************************************


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

#include "hci.h"
#include "btstack_util.h"
#include "bluetooth.h"
#include "bluetooth_gatt.h"

#include "ble/gatt-service/device_information_service_server.h"
#include "device_information_service_server_test.h"
#include "mock_att_server.h"

#define DEVICE_INFORMATION_CHARACTERISTIC_UUIDS_NUM 10
typedef struct {
    uint8_t * data;
    uint16_t  len;
    uint16_t  value_handle;
} device_information_field_t;

const uint16_t device_information_characteristic_uuids[] = {
        ORG_BLUETOOTH_CHARACTERISTIC_MANUFACTURER_NAME_STRING,
        ORG_BLUETOOTH_CHARACTERISTIC_MODEL_NUMBER_STRING,
        ORG_BLUETOOTH_CHARACTERISTIC_SERIAL_NUMBER_STRING,
        ORG_BLUETOOTH_CHARACTERISTIC_HARDWARE_REVISION_STRING,
        ORG_BLUETOOTH_CHARACTERISTIC_FIRMWARE_REVISION_STRING,
        ORG_BLUETOOTH_CHARACTERISTIC_SOFTWARE_REVISION_STRING,
        ORG_BLUETOOTH_CHARACTERISTIC_SYSTEM_ID,
        ORG_BLUETOOTH_CHARACTERISTIC_IEEE_11073_20601_REGULATORY_CERTIFICATION_DATA_LIST,
        ORG_BLUETOOTH_CHARACTERISTIC_PNP_ID,
        ORG_BLUETOOTH_CHARACTERISTIC_UDI_FOR_MEDICAL_DEVICES
};

TEST_GROUP(DEVICE_INFORMATION_SERVICE_SERVER){ 
    att_service_handler_t * service; 
    uint16_t con_handle;
    device_information_field_t device_information_fields[DEVICE_INFORMATION_CHARACTERISTIC_UUIDS_NUM];

    void setup(void){
        // setup database
        att_set_db(profile_data);
        // setup battery service
        device_information_service_server_init();

        int i;
        for (i=0;i<DEVICE_INFORMATION_CHARACTERISTIC_UUIDS_NUM;i++){
            device_information_fields[i].value_handle = gatt_server_get_value_handle_for_characteristic_with_uuid16(0, 0xffff, device_information_characteristic_uuids[i]);
        }

        device_information_service_server_set_manufacturer_name("manufacturer_name");
        device_information_service_server_set_model_number("model_number");
        device_information_service_server_set_serial_number("serial_number");
        device_information_service_server_set_hardware_revision("hardware_revision");
        device_information_service_server_set_firmware_revision("firmware_revision");
        device_information_service_server_set_software_revision("software_revision");

        device_information_service_server_set_system_id(0x01, 0x02);
        device_information_service_server_set_ieee_regulatory_certification(0x03, 0x04);
        device_information_service_server_set_pnp_id(0x05, 0x06, 0x07, 0x08);
        device_information_service_server_set_udi_for_medical_devices("label", "device_id", "issuer", "authority");

        service = mock_att_server_get_service();
        con_handle = 0x00;
    }

    void teardown(){
        mock().clear();
    }
};

TEST(DEVICE_INFORMATION_SERVICE_SERVER, lookup_attribute_handles){
    // get characteristic value handles
    int i;
    for (i=0;i<DEVICE_INFORMATION_CHARACTERISTIC_UUIDS_NUM;i++){
        CHECK(device_information_fields[i].value_handle != 0);
    }
}

TEST(DEVICE_INFORMATION_SERVICE_SERVER, read_values){
    uint8_t response[8];
    uint16_t response_len;

    // invalid attribute handle
    response_len = mock_att_service_read_callback(con_handle, 0xffff, 0xffff, response, sizeof(response));
    CHECK_EQUAL(0, response_len);

    int i;
    for (i=0;i<DEVICE_INFORMATION_CHARACTERISTIC_UUIDS_NUM;i++){
        response_len = mock_att_service_read_callback(con_handle, device_information_fields[i].value_handle, 0, response, sizeof(response));
        CHECK(response_len > 0);
    }

    // NULL data field
    device_information_service_server_set_manufacturer_name(NULL);
    response_len = mock_att_service_read_callback(con_handle, device_information_fields[0].value_handle, 0, response, sizeof(response));
    CHECK(response_len == 0);


    // invalid handle
    response_len = mock_att_service_read_callback(con_handle, 0xFFFF, 0, response, sizeof(response));
    CHECK(response_len == 0);
}

TEST(DEVICE_INFORMATION_SERVICE_SERVER, read_udi_for_medical_devices){
    uint8_t response[8];
    uint16_t response_len;
    
    uint8_t udi_index = DEVICE_INFORMATION_CHARACTERISTIC_UUIDS_NUM - 1;

    device_information_service_server_set_udi_for_medical_devices(NULL, NULL, NULL, NULL);
    response_len = mock_att_service_read_callback(con_handle, device_information_fields[udi_index].value_handle, 0, response, sizeof(response));
    CHECK(response_len > 0);

}

int main (int argc, const char * argv[]){
    return CommandLineTestRunner::RunAllTests(argc, argv);
}

void bt_reset();
void bt_init();
void bt_loadfirmware();
void setLEeventmask(unsigned char mask);
void startActiveScanning();
unsigned int bt_isReadByteReady();
unsigned char bt_readByte();

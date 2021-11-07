void spi_init();
void spi_send_recv(unsigned char *sbuffer, unsigned char *rbuffer, unsigned int size);
void spi_send(unsigned char *data, unsigned int size);
void spi_recv(unsigned char *data, unsigned int size);
void spi_chip_select(unsigned char chip_select);

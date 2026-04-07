#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#define w_len 1
#define r_len_col 8
#define r_len_acc 6
#define CMD_BIT 0x80
#define MAX_COUNT 44032 //máximo según ATIME (0xD5 -> 44032)
#define sensibilidad 16384.0

// Prepare messages (write + read)
struct i2c_rdwr_ioctl_data packets;
struct i2c_msg messages[2];
char write_bytes[w_len];
char wake[2];
char sensib[1];
char enable[2];
char atime[2];
char control[2];
char id_reg[1];
char read_bytes_acc[r_len_acc];
char read_bytes_col[r_len_col];

uint8_t r_8bit, g_8bit, b_8bit;

int main() {
     char i2cFile[15];
     int addr_col = 0x29;
     int addr_acc = 0x68;
     int dev = 1;

     sprintf(i2cFile, "/dev/i2c-%d", dev);
     int fd = open(i2cFile, O_RDWR);
     ioctl(fd, I2C_SLAVE, addr_col);
     ioctl(fd, I2C_SLAVE, addr_acc);

 //INITIALIZATION ACCELEROMETER
     wake[0] = 0x6B;
     wake[1] = 0x00;

     // Message WRITE: send the registre that we want to read
     messages[0].addr   = addr_acc;
     messages[0].flags  = 0; // 0 = write
     messages[0].len    = 2;
     messages[0].buf    = wake;//para despertar al registro 107

     // Construct and send packets
     packets.msgs = messages;
     packets.nmsgs = 1;
     ioctl(fd, I2C_RDWR, &packets); //executes read and write messages

     usleep(100000);//100ms
     write_bytes[0] = 0x1C;//leemos la sensibilidad

     // Message WRITE: send the registre that we want to read
       messages[0].addr   = addr_acc;
       messages[0].flags  = 0; // 0 = write
       messages[0].len    = w_len;
       messages[0].buf    = write_bytes;

       // Message READ: read N bytes from the sensor
       messages[1].addr   = addr_acc;
       messages[1].flags  = I2C_M_RD;  // read
       messages[1].len    = 1;
       messages[1].buf    = sensib;

       // Construct and send packets
       packets.msgs = messages;
       packets.nmsgs = 2;
       ioctl(fd, I2C_RDWR, &packets); //executes read and write messages

      int8_t escala = sensib[0];
      printf("Escala: %d\n", escala);


 //INITIALIZATION COLOUR SENSOR
     //Leer ID del sensor (registro 0x12, con command bit 0x80)
     write_bytes[0] = 0x12 | CMD_BIT;

     messages[0].addr   = addr_col;
     messages[0].flags  = 0;
     messages[0].len    = w_len;
     messages[0].buf    = write_bytes;

     messages[1].addr   = addr_col;
     messages[1].flags  = I2C_M_RD;
     messages[1].len    = 1;
     messages[1].buf    = id_reg;

     packets.msgs = messages;
     packets.nmsgs = 2;
     ioctl(fd, I2C_RDWR, &packets);

     printf("Sensor ID: 0x%02X\n", (unsigned char)id_reg[0]);

     usleep(3000); // esperar al encendido

     // ENABLE = 0x03 -> PON + AEN
     enable[0] = 0x00 | CMD_BIT;//register
     enable[1] = 0x03;//value we want to put in the register

     messages[0].addr   = addr_col;
     messages[0].flags  = 0;
     messages[0].len    = 2;
     messages[0].buf    = enable;

     packets.msgs = messages;
     packets.nmsgs = 1;
     ioctl(fd, I2C_RDWR, &packets);

     // ATIME = 0xD5 -> ~100 ms de integración
     atime[0] = 0x01 | CMD_BIT; //El ATIME da tiempo al sensor para mirar y poner el valor verdadero
     atime[1] = 0xD5;

     messages[0].addr   = addr_col;
     messages[0].flags  = 0;
     messages[0].len    = 2;
     messages[0].buf    = atime;

     packets.msgs = messages;
     packets.nmsgs = 1;
     ioctl(fd, I2C_RDWR, &packets);
/*
     // CONTROL = 0x01 -> ganancia 4x
     control[0] = 0x0F | CMD_BIT; //solo si hay poca luz
     control[1] = 0x01;

     messages[0].addr   = addr_col;
     messages[0].flags  = 0;
     messages[0].len    = 2;
     messages[0].buf    = control;

     packets.msgs = messages;
     packets.nmsgs = 1;
     ioctl(fd, I2C_RDWR, &packets);
*/
     usleep(120000);  //esperar primera integración

     while(1)
     {
          //ACCELEROMETER
       write_bytes[0] = 0x3B; //to read X[15:8]

       // Message WRITE: send the registre that we want to read
       messages[0].addr   = addr_acc;
       messages[0].flags  = 0; // 0 = write
       messages[0].len    = w_len;
       messages[0].buf    = write_bytes;

       // Message READ: read N bytes from the sensor
       messages[1].addr   = addr_acc;
       messages[1].flags  = I2C_M_RD;  // read
       messages[1].len    = r_len_acc;
       messages[1].buf    = read_bytes_acc;

       // Construct and send packets
       packets.msgs = messages;
       packets.nmsgs = 2;
       ioctl(fd, I2C_RDWR, &packets); //executes read and write messages

       // Process data
       float x = (float)((int16_t)(read_bytes_acc[0] << 8 | read_bytes_acc[1]) / sensibilidad);
       float y = (float)((int16_t)(read_bytes_acc[2] << 8 | read_bytes_acc[3]) / sensibilidad);
       float z = (float)((int16_t)(read_bytes_acc[4] << 8 | read_bytes_acc[5]) / sensibilidad);

       printf("ACCELEROMETER:  X: %.3f, Y: %.3f, Z: %.3f\n", x, y, z);
       sleep(1);//esperamos 1s antes de medir color



          //COLOUR SENSOR
       write_bytes[0] = 0x14 | CMD_BIT;// Empezamos a leer desde CDATAL = 0x14

       // Message WRITE: send the registre that we want to read
       messages[0].addr   = addr_col;
       messages[0].flags  = 0; // 0 = write
       messages[0].len    = w_len;
       messages[0].buf    = write_bytes;

       // Message READ: read 8 bytes from the sensor
       messages[1].addr   = addr_col;
       messages[1].flags  = I2C_M_RD;  // read
       messages[1].len    = r_len_col;
       messages[1].buf    = read_bytes_col;

       // Construct and send packets
       packets.msgs = messages;
       packets.nmsgs = 2;
       ioctl(fd, I2C_RDWR, &packets);

       // Process data
       uint16_t clear = (uint16_t)((read_bytes_col[1] << 8) | (unsigned char)read_bytes_col[0]);
       uint16_t red   = (uint16_t)((read_bytes_col[3] << 8) | (unsigned char)read_bytes_col[2]);
       uint16_t green = (uint16_t)((read_bytes_col[5] << 8) | (unsigned char)read_bytes_col[4]);
       uint16_t blue  = (uint16_t)((read_bytes_col[7] << 8) | (unsigned char)read_bytes_col[6]);

       printf("COLOUR SENSOR:  C: %u, R: %u, G: %u, B: %u\n", clear, red, green, blue);

       /* // Usamos float para no perder decimales en la división
       r_8bit = (uint8_t)((float)red   / clear * 255.0);
       g_8bit = (uint8_t)((float)green / clear * 255.0);
       b_8bit = (uint8_t)((float)blue  / clear * 255.0);

       printf("RGB 8-bit -> R: %d, G: %d, B: %d\n", r_8bit, g_8bit, b_8bit); */

       sleep(1); // esperamos 1s antes de repetir

     }

     close(fd);
     return 0;
}
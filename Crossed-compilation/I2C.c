#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
// I do all this include with chatgpt
#define w_len 1 //2 because its 16 bits register
#define r_len 6
#define sensibilidad 16384.0
// Prepare messages (write + read)
     struct i2c_rdwr_ioctl_data packets;
     struct i2c_msg messages[2];
     char write_bytes[w_len];
     char wake[2];
     char sensib[1];
     char read_bytes[r_len];

int main() {
     char i2cFile[15];
     int addr = 0x68;
     int dev = 1;
     sprintf(i2cFile, "/dev/i2c-%d", dev);
     int fd = open(i2cFile, O_RDWR);
     ioctl(fd, I2C_SLAVE, addr);

     wake[0] = 0x6B;
     wake[1] = 0x00;

     // Message WRITE: send the registre that we want to read
     messages[0].addr   = addr;
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
       messages[0].addr   = addr;
       messages[0].flags  = 0; // 0 = write
       messages[0].len    = w_len;
       messages[0].buf    = write_bytes;

       // Message READ: read N bytes from the sensor
       messages[1].addr   = addr;
       messages[1].flags  = I2C_M_RD;  // read
       messages[1].len    = 1;
       messages[1].buf    = sensib;

       // Construct and send packets
       packets.msgs = messages;
       packets.nmsgs = 2;
       ioctl(fd, I2C_RDWR, &packets); //executes read and write messages

      int8_t escala = sensib[0];
      printf("Escala: %d\n", escala);

     write_bytes[0] = 0x3B; //to read X[15:8]

     while(1)
     {
       // Message WRITE: send the registre that we want to read
       messages[0].addr   = addr;
       messages[0].flags  = 0; // 0 = write
       messages[0].len    = w_len;
       messages[0].buf    = write_bytes;

       // Message READ: read N bytes from the sensor
       messages[1].addr   = addr;
       messages[1].flags  = I2C_M_RD;  // read
       messages[1].len    = r_len;
       messages[1].buf    = read_bytes;

       // Construct and send packets
       packets.msgs = messages;
       packets.nmsgs = 2;
       ioctl(fd, I2C_RDWR, &packets); //executes read and write messages

       // Process data
       float x = (float)((int16_t)(read_bytes[0] << 8 | read_bytes[1]) / sensibilidad);
       float y = (float)((int16_t)(read_bytes[2] << 8 | read_bytes[3]) / sensibilidad);
       float z = (float)((int16_t)(read_bytes[4] << 8 | read_bytes[5]) / sensibilidad);

       printf("X: %.3f, Y: %.3f, Z: %.3f\n", x, y, z);
       sleep(1);//esperamos 5s antes de repetir
     }

     close(fd);
     return 0;
}


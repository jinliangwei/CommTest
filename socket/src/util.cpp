
#include <sys/types.h>
#include <stdint.h>
#include "util.hpp"
#include <stdio.h>
#include <unistd.h>

namespace commtest{

  int readn(int fd, uint8_t *buff, size_t len){
    size_t readbytes = read(fd, buff, len);
    LOG(DBG, stderr, "read %ld\n", readbytes);
    if(readbytes == 0) return 0;

    if(readbytes < 0){
      return -1;
    }

    size_t readmore;
    while(readbytes < len){
      readmore = read(fd, (buff + readbytes), (len - readbytes));
      
      if(readmore < 0){
	if(readbytes < 0) return -1;
      }else{
	readbytes += readmore;
      }

    }
    return readbytes;
  }

  int writen(int fd, uint8_t *buff, size_t len){
    size_t i = 0;
    while(i < len){
      int written = write(fd, buff + i, (len - i));
      if(written < 0) return -1;
      i += (size_t) written;
    }
    return i;
  }
};

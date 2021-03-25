/*
 * Copyright (c) 2020 Louis Suen
 * Licensed under the MIT License. See the LICENSE file for the full text.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdint.h>
#include <inttypes.h>
#include <sys/mman.h>



#include "log.h"
#include "tab_gbk2uni.h"


/*
 * Determine the current offset and remaining length of the open file.
 */
static int get_file_start_and_length(int fd, off_t *start_, size_t *length_)
{
    off_t start, end;
    size_t length;

    //assert(start_ != NULL);
    //assert(length_ != NULL);

    start = lseek(fd, 0L, SEEK_CUR);
    end = lseek(fd, 0L, SEEK_END);
    (void) lseek(fd, start, SEEK_SET);

    if (start == (off_t) -1 || end == (off_t) -1) {
        LOGE("could not determine length of file\n");
        return -1;
    }

    length = end - start;
    if (length == 0) {
        LOGE("file is empty\n");
        return -1;
    }


    if (start_) {
          *start_ = start;
    }

    if (length_) {
      *length_ = length;
    }

    return 0;
}


int main(int argc, char *argv[])
{

	int ret = -1, err, rv;
	int fd = -1;
	char *filename = NULL;

  	void* mem_ptr = MAP_FAILED;
	size_t mem_size = 0;
	char *tmp_ptr = NULL;

	LOGD("argc = [%d]\n", argc);
	if (argc < 2 || NULL == argv[1] || strlen(argv[1]) <= 0) {
		LOGE("input param error!!!(It can not find filename.)\n");
		goto __oops;
	}
	
	filename = argv[1];
	fd = open(filename, O_RDONLY);
  	if (fd < 0) {
    		err = errno ? errno : -1;
    		LOGE("Unable to open '%s': %s\n", filename, strerror(err));
    		goto __oops;
  	}

	ret = get_file_start_and_length(fd, NULL, &mem_size);
	if (ret) {
    		LOGE("get_file_start_and_length error!\n");
    		goto __oops;
	}
	if (mem_size <= 0) {
    		LOGE("file(%s) length is 0!\n", filename);
    		goto __oops;
	}

  	LOGD("filename = [%d][%s][%zu]\n", fd, filename, mem_size);

  	mem_ptr = mmap(NULL, mem_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
  	if (mem_ptr == MAP_FAILED) {
    		err = errno ? errno : -1;
		LOGE("unable to mmap: %s\n", strerror(errno));
    		goto __oops;
  	}
	
	
	rv = is_valid_utf8((const unsigned char*)mem_ptr, strlen((char*)mem_ptr));
	LOGD("is_valid_utf8 = [%d]\n", rv);
	if (0 != rv) {
		if (0 != is_valid_gbk((const unsigned char*)mem_ptr, strlen((char*)mem_ptr))) {
            //既是utf-8又是gbk,按gbk处理
			rv = 0;	
		}
	}

	if (rv) {
		LOGD("mem_ptr = [%zu][%s]\n", strlen(mem_ptr), mem_ptr);
	} else {
		err = 0;
		tmp_ptr = gbk2utf8(mem_ptr, mem_size, &err);
		if (NULL == tmp_ptr) {
			LOGE("tmp_ptr = [%d][0][NULL]\n", err);
			//LOGD("mem_ptr's encoding is unknow! \n");
		} else {
			LOGD("tmp_ptr = [%d][%zu][%s]\n", err, strlen(tmp_ptr), tmp_ptr);
		}
	}

	

__oops:
	
	if (tmp_ptr) {
		free(tmp_ptr);
		tmp_ptr = NULL;
	}

  	if (mem_ptr != MAP_FAILED && mem_size > 0) {
		munmap(mem_ptr, mem_size);	
		mem_ptr = MAP_FAILED;
		mem_size = 0;
	}

	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	
	return 0;
}



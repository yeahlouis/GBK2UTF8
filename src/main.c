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

static int is_valid_utf8(const unsigned char *data, size_t len)
{
	//int ret = -1;
	int i, flg;
	const unsigned char *cur;

	if (NULL == data || len <= 0) {
		return 0;
	}

	flg = 1;

	for (i = 0, cur = data; i < len; i ++, cur ++) {
    		if ((*cur & 0x80) == 0) {
			/* 0xxxxxxx */
      			continue;
		} else if ((*cur & 0xE0) == 0xC0) {
			/* 110xxxxx 10xxxxxx */
			if (i + 1 >= len) {
				flg = 0;
				break;	
			}
			if ((*(cur + 1) & 0xC0) != 0x80) {
				flg = 0;
				break;	
			}
			i += 1;
			cur += 1;
			
		} else if ((*cur & 0xF0) == 0xE0) {
			/* 1110xxxx 10xxxxxx 10xxxxxx */
			if (i + 2 >= len) {
				flg = 0;
				break;	
			}
			if ((*(cur + 1) & 0xC0) != 0x80 || (*(cur + 2) & 0xC0) != 0x80) {
				flg = 0;
				break;	
			}
			i += 2;
			cur += 2;

		} else if ((*cur & 0xF8) == 0xF0) {
			/* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
			if (i + 3 >= len) {
				flg = 0;
				break;	
			}
			if ((*(cur + 1) & 0xC0) != 0x80 || (*(cur + 2) & 0xC0) != 0x80 || (*(cur + 3) & 0xC0) != 0x80) {
				flg = 0;
				break;	
			}
			i += 3;
			cur += 3;
		} else {
			flg = 0;
			break;

		}
	}
	
	return (flg);

}

static int is_valid_gbk(const unsigned char *data, size_t len)
{
	//int ret = -1;
	int i, flg;
	const unsigned char *cur;

	if (NULL == data || len <= 0) {
		return 0;
	}

	flg = 1;

	for (i = 0, cur = data; i < len; i ++, cur ++) {
    		if ((*cur & 0x80) == 0) {
			/* 0xxxxxxx */
      			continue;
		} else if (*cur > 0x80 && *cur < 0xFF) {
			if (i + 1 >= len) {
				flg = 0;
				break;	
			}
			i += 1;
			cur += 1;
			if (*cur < 0x40 || *cur == 0xFF || *cur == 0x7F) {
				flg = 0;
				break;	
			}

		} else {
			flg = 0;
			break;

		}
	}
	
	return (flg);

}
static int gbk_uni2utf8(unsigned short ns, unsigned char buf[4]) 
{
	//LOGD("ns = [%hu][%#hx]\n", ns, ns);
	//if (ns < 0x80 || NULL == buf) {
	if (ns < 0xFF || NULL == buf) { //Hanzi encoding greater than 0xFF
		return (-1);
	}	
#if 1
	if (ns == 0x0251 || ns == 0x0261 || ns == 0x02C9 || ns == 0x02C7 || 
	    ns == 0x02CA || ns == 0x02CB || ns == 0x02D9 || ns == 0x0401 || ns == 0x0451) {
		return (-1);
	}
	if (ns >= 0x0410 && ns <= 0x044F) {
		return (-1);
	}
#endif
	if (0 != (ns & 0xF100)) {
		buf[0] = (ns & 0xF000) >> 12 | 0xE0;
		buf[1] = (ns & 0x0FC0) >> 6  | 0x80;
		buf[2] = (ns & 0x003F)       | 0x80;
		buf[3] = 0;
	} else {
		buf[0] = (ns & 0x07C0) >> 6 | 0xE0;
		buf[1] = (ns & 0x003F)      | 0x80;
		buf[2] = 0;
		buf[3] = 0;
	}
	return 0;
}
static char *gbk2utf8(const unsigned char *data, size_t len)
{
	int rv;
	int i, j, flg, count;
	char *p_ret = NULL;
	unsigned char *p_cur = NULL, *p_src = NULL;
	const unsigned char *cur;
	unsigned char buf[4];

	if (NULL == data || len <= 0) {
		return NULL;
	}
	
	p_src = (unsigned char *)malloc(len * 2);
	if (NULL == p_src) {
		LOGE("malloc(%zd) failed!\n", len * 2);
		goto __oops;
	}

	flg = 1;

	for (i = 0, cur = data, p_cur = p_src; i < len && flg; i ++, cur ++) {
    		if ((*cur & 0x80) == 0) {
			/* 0xxxxxxx */
			*p_cur++ = *cur;
      			continue;
		} else if (*cur > 0x80 && *cur < 0xFF) {
			if (i + 1 >= len) {
				flg = 0;
				break;	
			}
			count = *cur - 0x81;
			i += 1;
			cur += 1;
			if (*cur < 0x40 || *cur == 0xFF || *cur == 0x7F) {
				flg = 0;
				break;	
			}
			count = count * 12 * 16 + *cur - 0x40;
			rv = gbk_uni2utf8(tab_uni_contents[count], buf);
			if (0 != rv) {
				flg = 0;
				break;	
			}			
			for (j = 0; j < sizeof(buf); j ++) {
				if (0 == buf[j]) 
					break;
				*p_cur++ = buf[j];
			}
		} else {
			flg = 0;
			break;

		}
	}

	if (0 != flg) {
		*p_cur = 0;
		p_ret = strdup((char *)p_src);
	}

__oops:
	if (p_src) {
		free(p_src);
		p_src = NULL;
	}

	return p_ret;
} 
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

	int ret = -1, err;
	int i, j;
	int fd = -1;
	FILE *fp = NULL;
	FILE *fpu = NULL;
	FILE *fpg = NULL;
	char *filename = NULL;

  	void* mem_ptr = MAP_FAILED;
	size_t mem_size = 0;

	LOGD("argc = [%d]\n", argc);
	if (argc < 2 || NULL == argv[1] || strlen(argv[1]) <= 0) {
		LOGE("input param error!!!(It can not find filename.)\n");
		goto __oops;
	}
	
	filename = argv[1];
	fd = open(filename, O_RDWR);
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

  	LOGD("filename = [%d][%s][%zu]\n", fd, filename, mem_size);

  	mem_ptr = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd, 0);
  	if (mem_ptr == MAP_FAILED) {
    		err = errno ? errno : -1;
		LOGE("unable to mmap: %s\n", strerror(errno));
    		goto __oops;
  	}
	
	
        fp = fopen("bbb.txt", "w");
        if (fp == NULL) {
        	LOGE("ERROR: unable to open %s\n", "bbb.txt");
    		goto __oops;
	}   

        fpu = fopen("uuu.txt", "w");
        if (fpu == NULL) {
        	LOGE("ERROR: unable to open %s\n", "uuu.txt");
    		goto __oops;
	}   

        fpg = fopen("ggg.txt", "w");
        if (fpg == NULL) {
        	LOGE("ERROR: unable to open %s\n", "ggg.txt");
    		goto __oops;
	}   
#if 0
	LOGD("file contents:\n%s\n", (char *)mem_ptr);
#endif

	ret = is_valid_utf8((const unsigned char*)mem_ptr, strlen((char*)mem_ptr));
	LOGD("is_valid_utf8 = [%d]\n", ret);

	ret = is_valid_gbk((const unsigned char*)mem_ptr, strlen((char*)mem_ptr));
	LOGD("is_valid_gbk = [%d]\n", ret);

	//LOGD("tab_gbk_contents = [%zu]\n", sizeof(tab_gbk_contents) / sizeof(tab_gbk_contents[0]));
	LOGD("tab_uni_contents = [%zu]\n", sizeof(tab_uni_contents) / sizeof(tab_uni_contents[0]));

	//static char *gbk2utf8(const unsigned char *data, size_t len)
	//unsigned char aa[2] = {0xce, 0xd2};
	//char *aaa = gbk2utf8(aa, sizeof(aa));
	//LOGE("cede = [%zu][%s]\n", strlen(aaa), aaa);
	
	char *aaa = gbk2utf8(mem_ptr, mem_size);
	LOGE("cede = [%zu][%s]\n", strlen(aaa), aaa);

	
	unsigned char bb[2] = {0xce, 0xd2};
	int k = 0;
	int nn = 0;

	for (i = 0x81; i < 0xFF; i ++) {
	//for (i = 0xA1; i < 0xBF; i ++) {
		bb[0] = i & 0xFF;	
		for (j = 0x40; j <= 0xFF; j ++) {
			bb[1] = j & 0xFF;	
			int index = (bb[0] - 0x81) * 12 * 16 + (bb[1] - 0x40);
			char *bbb = gbk2utf8(bb, sizeof(bb));
			if (bbb) {
				fprintf(fp, "k = [%d],[0x%02X%02X]\n", k++, bb[0], bb[1]);
				fprintf(fp, "bbb = [%zu][%s][%04hX]\n", strlen(bbb), bbb, tab_uni_contents[index]);
				fprintf(fpu, "%s", bbb);
				fwrite(bb, 2, 1, fpg);
				nn ++;
				if (nn > 0 && nn % 64 == 0) {
					fprintf(fpu, "%s", "\n");
					fprintf(fpg, "%s", "\n");
				}
			}
		}
	}

__oops:

  	if (mem_ptr != MAP_FAILED && mem_size > 0) {
		munmap(mem_ptr, mem_size);	
		mem_ptr = MAP_FAILED;
		mem_size = 0;
	}

	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
	
	if (fp) {
		fclose(fp);
		fp = NULL;
	}
	if (fpu) {
		fclose(fpu);
		fpu = NULL;
	}

	return 0;
}



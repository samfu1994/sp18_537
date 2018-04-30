#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <dirent.h>
#include <stdbool.h>

#define stat xv6_stat
#define dirent xv6_dirent  // avoid clash with host struct stat

#include "include/fs.h"
#include "include/stat.h"

int fsfd;
struct superblock * sb;
int nblocks, ninodes;
int * block_refs;
char mask;

void rsect(uint, void *);
void add_ref(struct dinode * cur_inode);

uint
i2b(uint inum)
{
  return (inum / IPB) + 2;
}

ushort
xshort(ushort x)
{
  ushort y;
  char *a = (char*)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint
xint(uint x)
{
  uint y;
  char *a = (char*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}

void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * 512L, 0) != sec * 512L){
    perror("lseek");
    exit(1);
  }
  if(read(fsfd, buf, 512) != 512){
    perror("read");
    exit(1);
  }
}

char uget_c(uint * arr, int i, char * bitmap) {
	int addr = xint(arr[i]);
	// block_refs[addr] += 1;
	int index = addr / 8;
	int bit = addr % 8;
	char c = bitmap[index] >> bit;
	return c;
}

char get_c(int * arr, int i, char * bitmap) {
	int addr = xint(arr[i]);
	// block_refs[addr] += 1;
	int index = addr / 8;
	int bit = addr % 8;
	char c = bitmap[index] >> bit;
	return c;
}

char get_c_with_index(int i, char * bitmap) {
	int index = i / 8;
	int bit = i % 8;
	char c = bitmap[index] >> bit;
	return c;
}

void check_bitmap(struct dinode * cur_inode) {
	int start = ninodes / IPB + 3;
	for(int i = 0; i <= start; i++) {
		block_refs[i] = 1;
	}
	char bitmap[512];
	rsect(start, bitmap);

	int off = xint(cur_inode -> size);
	int fbn = off / 512 + 1;
	int bound = fbn < NDIRECT ? fbn : NDIRECT;
	uint indirect[NINDIRECT];
	char c;

	for(int i = 0; i < bound; i++) {
		c = uget_c(cur_inode -> addrs, i, bitmap);
		if(!(c & mask)) {
			fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
			exit(1);
		}
	}
	//indirect
	if(fbn >= NDIRECT) {
		int base = xint(cur_inode -> addrs[NDIRECT]);
		rsect(base, (char*)indirect);
		block_refs[base] += 1;
		for(int i = 0; i < NINDIRECT; i++) {
			c = uget_c(indirect, i, bitmap);
			if(!(c & mask)) {
				fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
				exit(1);
			}
		}
	}
}

void check_reference_count() {

}

void check_bitmap_marks_free_inused() {
	int start = ninodes / IPB + 3;
	char bitmap[512];
	rsect(start, bitmap);
	char c;
	//reverse

	for(int i = start + 1; i < sb -> nblocks; i++) {
		if(block_refs[i] >= 1) continue;
		c = get_c_with_index(i, bitmap);
		if(c & mask) {
			printf("i is %d\n", i);
			fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
			exit(1);
		}
	}
}

void check_self_parent(struct dinode * cur_inode, int inum) {
	if(cur_inode -> type != T_DIR) {
		return;
	}
	//read first two memory piece of size sizeof(xv6_dirent)
	struct xv6_dirent * de;
	char buf[512];
	int block = cur_inode -> addrs[0];
	rsect(block, buf);
	de = (struct xv6_dirent*)buf;
	if(strcmp(de -> name, ".") != 0) {
		fprintf(stderr, "ERROR: directory not properly formatted.\n");
		exit(1);
	}
	if(de -> inum != inum) {
		fprintf(stderr, "ERROR: directory not properly formatted.\n");
		exit(1);
	}
	de += 1;
	if(strcmp(de -> name, "..") != 0) {
		fprintf(stderr, "ERROR: directory not properly formatted.\n");
		exit(1);
	}
	if(inum == 1) {
		if(de -> inum != 1) {
			fprintf(stderr, "ERROR: root directory does not exist.\n");
			exit(1);
		}
	}
}

void check_type(struct dinode * cur_inode, int block_index, int inode_index) {
	short cur_type;
	cur_type = cur_inode -> type;
	if(block_index == 2 && inode_index == 1) {
		//root inode
		if(cur_type != T_DIR) {
			fprintf(stderr, "ERROR: root directory does not exist.\n");
			exit(1);
		}

	}
	else{
		if(cur_type != T_DIR && cur_type != T_FILE && cur_type != T_DEV) {
			fprintf(stderr, "ERROR: bad inode.\n");
			exit(1);
		}
	}
}

void check_addr(struct dinode * cur_inode) {
	int off = xint(cur_inode -> size);
	int fbn = off / 512 + 1;
	int bound = fbn < NDIRECT ? fbn : NDIRECT;
	uint indirect[NINDIRECT];

	for(int i = 0; i < bound; i++) {
		printf("direct : addr is %u\n", cur_inode -> addrs[i]);
		if(xint(cur_inode -> addrs[i]) >= nblocks) {
			fprintf(stderr, "ERROR: bad direct address in inode.\n");
			exit(1);
		}
	}
	//indirect
	if(fbn >= NDIRECT) {
		rsect(xint(cur_inode -> addrs[NDIRECT]), (char*)indirect);
		printf("indirect : INTER is %u\n", cur_inode -> addrs[NDIRECT]);
		for(int i = 0; i < NINDIRECT; i++) {
			int cur = indirect[i];
			printf("indirect : addr is %u\n", cur);
			if(cur >= nblocks) {
				fprintf(stderr, "ERROR: bad indirect address in inode.\n");
				exit(1);
			}
		}
	}
}

void add_ref_file_inode(struct dinode * cur_inode) {
	int off = xint(cur_inode -> size);
	int fbn = off / 512 + 1;
	int bound = fbn < NDIRECT ? fbn : NDIRECT;
	uint indirect[NINDIRECT];
	for(int i = 0; i < bound; i++) {
		int target = cur_inode -> addrs[i];
		block_refs[target] += 1;
	}

		//indirect
	if(fbn >= NDIRECT) {
		int base = xint(cur_inode -> addrs[NDIRECT]);
		rsect(base, (char*)indirect);
		block_refs[base] += 1;
		for(int i = 0; i < NINDIRECT; i++) {
			block_refs[indirect[i]] += 1;
		}
	}
}

void read_dir_buf(char * buf) {
	// int size_dinode = sizeof(struct dinode);
	int finished = 0;
	int size_dirent = sizeof(struct dirent);
	struct dirent * pdirent = (struct dirent *)buf;
	struct dinode * pdinode;
	char inode_buf[512];
	while(1) {
		if(pdirent -> inum == 0) {
			break;
		}
		if(strcmp(pdirent -> name, ".") == 0) {
			pdirent += 1;
			finished += size_dirent;
			continue;
		}
		if(strcmp(pdirent -> name, "..") == 0) {
			pdirent += 1;
			finished += size_dirent;
			continue;
		}
		int b = i2b(pdirent -> inum);
		rsect(b, inode_buf);
		int off = pdirent -> inum % IPB;
		pdinode = ((struct dinode *)inode_buf) + off;
		add_ref(pdinode);
		pdirent += 1;
		finished += size_dirent;
		if(finished == 512) {
			break;
		}
	}
	return;
}

void add_ref(struct dinode * cur_inode) {
	if(cur_inode -> type == T_FILE) {
		add_ref_file_inode(cur_inode);
	}
	else if(cur_inode -> type == T_DIR) {
		int off = xint(cur_inode -> size);
		int fbn = off / 512 + 1;
		int bound = fbn < NDIRECT ? fbn : NDIRECT;
		uint indirect[NINDIRECT];
		char indirect_buf[512];
		int finished = 0;

		char buf[512];
		for(int i = 0; i < bound; i++) {
			int target = cur_inode -> addrs[i];
			block_refs[target] += 1;
			rsect(target, buf);
			read_dir_buf(buf);
			finished += 512;
		}

		if(fbn >= NDIRECT) {
			int base = xint(cur_inode -> addrs[NDIRECT]);
			rsect(base, (char*)indirect);
			for(int i = 0; i < NINDIRECT && finished < cur_inode -> size; i++) {
				rsect(indirect[i], indirect_buf);
				block_refs[indirect[i]] += 1;
				read_dir_buf(indirect_buf);
				finished += 512;
			}
		}
	}
}

void print_bitmap() {
	for(int i = 0; i < sb -> nblocks; i++) {
		printf("block[%d] : %d\n", i, block_refs[i]);
	}

	int start = ninodes / IPB + 3;
	char bitmap[512];
	rsect(start, bitmap);
	char c;
	for(int i = 0; i < 512; i++) {
		printf("%d : ", i);
		for(int j = 0; j < 8; j++) {
			c = bitmap[i];
			if(mask & (c >> j) ) {
				printf("1 ");
			}
			else{
				printf("0 ");
			}
		}
		printf("\n");
	}
}


int main(int argc, char * argv[]){
	if(argc < 2) {
		fprintf(stderr, "Usage: xcheck <file_system_image>\n");
		exit(1);
	}
	mask = 0x01;
	char buf[512], sb_buf[512];
	char * img_name = argv[1];
	fsfd = open(img_name, O_RDONLY);
	if(fsfd == -1) {
		fprintf(stderr, "image not found.\n");
		exit(1);
	}

	struct dinode * cur_inode;

	rsect(1, sb_buf);
	sb = (struct superblock *) sb_buf;
	printf("size : %d, nblocks : %d, ninodes : %d\n", sb -> size, sb -> nblocks, sb -> ninodes);
	nblocks = sb -> nblocks;
	ninodes = sb -> ninodes;

	block_refs = malloc(sizeof(int) * nblocks);

	int inode_read = 0;
	int quit = 0;
	for(int i = 0; i < sb -> ninodes / IPB + 1; i++) {
		rsect(i + 2, buf);
		for(int j = 0; j < IPB; j++) {
			if(inode_read == ninodes) {
				quit = 1;
				break;
			}
			if(i == 0 && j == 0) continue;
			cur_inode = ((struct dinode *)buf) + j;
			if(cur_inode -> nlink < 1){
				quit = 1;
				break;
			}
			printf("inode : %d, link :%hd, size %u, type: %hd\n", 
				inode_read + 1, cur_inode -> nlink,  cur_inode -> size, cur_inode -> type);
			check_type(cur_inode, i + 2, j);
			check_addr(cur_inode);
			check_self_parent(cur_inode, inode_read + 1);
			check_bitmap(cur_inode);
			add_ref(cur_inode);
			inode_read += 1;
		}
		if(quit) {
			break;
		}
	}
	check_bitmap_marks_free_inused();
	print_bitmap();


	free(block_refs);

	close(fsfd);
    return 0;
}

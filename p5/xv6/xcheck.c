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

void rsect(uint, void *);

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

void check_bitmap(struct dinode * cur_inode) {
	int start = ninodes / IPB + 3;
	for(int i = 0; i <= start; i++) {
		block_refs[i] = 1;
	}
	char bitmap[512];
	rsect(start, bitmap);

	int off = xint(cur_inode -> size);
	int fbn = off / 512;
	int bound = fbn < NDIRECT ? fbn : NDIRECT;
	uint indirect[NINDIRECT];
	int index, bit, addr;
	char c, mask = 0x01;

	for(int i = 0; i < bound; i++) {
		addr = xint(cur_inode -> addrs[i]);
		block_refs[addr] += 1;
		if(addr > start && block_refs[addr] > 1) {
			fprintf(stderr, "ERROR: direct address used more than once.\n");
			exit(1);
		}
		// printf("addr is %d\n", addr);
		index = addr / 8;
		bit = addr % 8;
		c = bitmap[index] >> bit;
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
		if(base > start && block_refs[base] > 1) {
			fprintf(stderr, "ERROR: indirect address used more than once.\n");
			exit(1);
		}
		for(int i = 0; i < NINDIRECT; i++) {
			addr = indirect[i];
			block_refs[addr] += 1;
			if(addr > start && block_refs[addr] > 1) {
				fprintf(stderr, "ERROR: indirect address used more than once.\n");
				exit(1);
			}
			index = addr / 8;
			bit = addr % 8;
			c = bitmap[index] >> bit;
			if(!(c & mask)) {
				fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
				exit(1);
			}
		}
	}
}

void check_reference_count() {

}

void check_bitmap_marks_free_inused(struct dinode * cur_inode) {
	int start = ninodes / IPB + 3;
	char bitmap[512];
	rsect(start, bitmap);

	int index, bit;
	char mask, c;
	//reverse
	for(int i = 0; i < sb -> nblocks; i++) {
		printf("ref[%d] is %d\n", i, block_refs[i]);
	}
	for(int i = start + 1; i < sb -> nblocks; i++) {
		if(block_refs[i] == 1) continue;
		index = i / 8;
		bit = i % 8;
		mask = 0x01;
		c = bitmap[index] >> bit;
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
	int fbn = off / 512;
	int bound = fbn < NDIRECT ? fbn : NDIRECT;
	uint indirect[NINDIRECT];

	for(int i = 0; i < bound; i++) {
		if(xint(cur_inode -> addrs[i]) >= nblocks) {
			// printf("addr is %u\n", cur_inode -> addrs[i]);
			fprintf(stderr, "ERROR: bad direct address in inode.\n");
			exit(1);
		}
	}
	//indirect
	if(fbn >= NDIRECT) {
		rsect(xint(cur_inode -> addrs[NDIRECT]), (char*)indirect);
		for(int i = 0; i < NINDIRECT; i++) {
			int cur = indirect[i];
			if(cur >= nblocks) {
				// printf("addr is %u\n", cur_inode -> addrs[i]);
				fprintf(stderr, "ERROR: bad indirect address in inode.\n");
				exit(1);
			}
		}
	}
}

int main(int argc, char * argv[]){
	if(argc < 2) {
		fprintf(stderr, "Usage: xcheck <file_system_image>\n");
		exit(1);
	}
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
			inode_read += 1;
		}
		if(quit) {
			break;
		}
	}

	free(block_refs);

	printf("finish first round\n");

	// for(int i = 0; i < sb -> ninodes / IPB + 1; i++) {
	// 	rsect(i + 2, buf);
	// 	for(int j = 0; j < IPB; j++) {
	// 		if(inode_read == ninodes) {
	// 			quit = 1;
	// 			break;
	// 		}
	// 		if(i == 0 && j == 0) continue;
	// 		cur_inode = ((struct dinode *)buf) + j;
	// 		if(cur_inode -> nlink < 1){
	// 			quit = 1;
	// 			break;
	// 		}
	// 		check_bitmap_marks_free_inused(cur_inode);
	// 		inode_read += 1;
	// 	}
	// 	if(quit) {
	// 		break;
	// 	}
	// }


	close(fsfd);
    return 0;
}

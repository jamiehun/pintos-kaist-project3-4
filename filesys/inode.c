#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/fat.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* On-disk inode.
 * Must be exactly DISK_SECTOR_SIZE bytes long. */
struct inode_disk
{
	disk_sector_t start;  /* First data sector. */
	off_t length;		  /* File size in bytes. */
	unsigned magic;		  /* Magic number. */
	uint32_t unused[125]; /* Not used. */
};

/* Returns the number of sectors to allocate for an inode SIZE
 * bytes long. */
static inline size_t
bytes_to_sectors(off_t size)
{
	return DIV_ROUND_UP(size, DISK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode
{
	struct list_elem elem;	/* Element in inode list. */
	disk_sector_t sector;	/* Sector number of disk location. (inode_disk가 저장되어 있는 disk의 sector) */
	int open_cnt;			/* Number of openers. (inode가 열려있으면 닫으면 안됨) */
	bool removed;			/* True if deleted, false otherwise. */
	int deny_write_cnt;		/* 0: writes ok, >0: deny writes. */
	struct inode_disk data; /* Inode content.(물리메모리에 올려 놓은 데이터) */
};

/* Returns the disk sector that contains byte offset POS within
 * INODE.
 * Returns -1 if INODE does not contain data for a byte at offset
 * POS. */
static disk_sector_t
byte_to_sector(const struct inode *inode, off_t pos)
{
	ASSERT(inode != NULL);
	
	cluster_t target = sector_to_cluster(inode->data.start); // inode의 data가 시작하는 곳에서부터 cluster 시작

	/* file length와 관계없이 pos의 크기에 따라 계속 진행 
	   file length보다 pos가 크면 새로운 cluster를 할당해가면서 진행 */
	while (pos >= DISK_SECTOR_SIZE)
	{
		// pos가 기존보다 클 경우 chain을 확장
		if (fat_get(target) == EOChain)
			fat_create_chain(target);

		// target을 update 및 offset을 DISK_SECTOR_SIZE만큼 차감
		target = fat_get(target);
		pos -= DISK_SECTOR_SIZE;
	}

	disk_sector_t sector = cluster_to_sector(target);
	return sector;
}

/* List of open inodes, so that opening a single inode twice
 * returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void inode_init(void)
{
	list_init(&open_inodes);
}

/* Sector에 length 바이트 만큼의 inode를 만들어줌*/
/* !!! 변경필요 !!! */
/* Initializes an inode with LENGTH bytes of data and
 * writes the new inode to sector SECTOR on the file system
 * disk.
 * Returns true if successful.
 * Returns false if memory or disk allocation fails. */
bool inode_create(disk_sector_t sector, off_t length)
{
	/* disk_inode를 초기화하기 위해 만들어줌*/
	struct inode_disk *disk_inode = NULL; // on-disk inode를 하나 만들어줌
	cluster_t start_clst;
	bool success = false;

	ASSERT(length >= 0);

	/* If this assertion fails, the inode structure is not exactly
	 * one sector in size, and you should fix that. */
	ASSERT(sizeof *disk_inode == DISK_SECTOR_SIZE);

	disk_inode = calloc(1, sizeof *disk_inode);  
	if (disk_inode != NULL)
	{
		size_t sectors = bytes_to_sectors(length); // length byte를 기준으로 sectors(sector의 개수)를 만들어줌
		disk_inode->length = length;
		disk_inode->magic = INODE_MAGIC;

		// if (free_map_allocate(sectors, &disk_inode->start)) // start부터 sectors만큼 할당, 저장을 시작 (!!!변경필요)

		/* 변경 후 */
		if (start_clst = fat_create_chain(0)) // start a new chain
		{
			disk_inode->start = cluster_to_sector(start_clst);
			disk_write(filesys_disk, sector, disk_inode); // disk_inode -> filesys_disk를 write (sector에)
			if (sectors > 0)
			{
				static char zeros[DISK_SECTOR_SIZE];
				cluster_t target = start_clst;
				disk_sector_t w_sector;
				size_t i;


				// for (i = 0; i < sectors; i++)
				// 	disk_write(filesys_disk, disk_inode->start + i, zeros); // 연속적으로 write 해 줌
					// 위의 disk_write랑 다른 점은 위의 disk_write는 disk_inode를 sector에 저장해주기 위한 함수이고,
					// 아래의 disk_write는 앞으로 써 나갈 내용을 sectors에 0으로 저장을 해놓는 것으로 보임
				
				/* 변경 후 */
				/* sectors의 개수만큼 zero로 disk write하며 fat_create_chain(확장시켜줌) */
				while (sectors > 0){
					w_sector = cluster_to_sector(target);
					disk_write(filesys_disk, w_sector, zeros);

					target = fat_create_chain(target);
					sectors--;
				}
			}
			success = true;
		}
		free(disk_inode); // 저장을 완료한 disk_inode를 free 
	}
	return success;
}

/* Reads an inode from SECTOR
 * and returns a `struct inode' that contains it.
 * Returns a null pointer if memory allocation fails. */
struct inode *
inode_open(disk_sector_t sector)
{
	struct list_elem *e;
	struct inode *inode;

	/* 이미 열려있으면 reopen 후 return */
	/* Check whether this inode is already open. */
	for (e = list_begin(&open_inodes); e != list_end(&open_inodes);
		 e = list_next(e))
	{
		inode = list_entry(e, struct inode, elem);
		if (inode->sector == sector)
		{
			inode_reopen(inode);
			return inode;
		}
	}

	/* 열려있지 않으면 새로운 inode를 malloc */
	/* Allocate memory. */
	inode = malloc(sizeof *inode);
	if (inode == NULL)
		return NULL;

	/* Initialize. */
	/* 위에서 malloc한 inode에 대한 값을 설정해줌 */
	list_push_front(&open_inodes, &inode->elem);
	inode->sector = sector;
	inode->open_cnt = 1;
	inode->deny_write_cnt = 0;
	inode->removed = false;
	disk_read(filesys_disk, inode->sector, &inode->data);
	return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen(struct inode *inode)
{
	if (inode != NULL)
		inode->open_cnt++;
	return inode;
}

/* Returns INODE's inode number. */
disk_sector_t
inode_get_inumber(const struct inode *inode)
{
	return inode->sector;
}

/* Closes INODE and writes it to disk.
 * If this was the last reference to INODE, frees its memory.
 * If INODE was also a removed inode, frees its blocks. */
void inode_close(struct inode *inode)
{
	/* Ignore null pointer. */
	if (inode == NULL)
		return;

	/* Release resources if this was the last opener. */
	if (--inode->open_cnt == 0)
	{
		/* Remove from inode list and release lock. */
		list_remove(&inode->elem);

		/* Deallocate blocks if removed. */
		if (inode->removed)
		{
			// free_map_release(inode->sector, 1); // inode_disk를 free
			// free_map_release(inode->data.start, bytes_to_sectors(inode->data.length)); // sectors를 전부 free (!!! 수정필요)
			
			/* 변경 후 */
			cluster_t clst = sector_to_cluster(inode->sector);
			fat_remove_chain(clst, 0);

			clst = sector_to_cluster(inode->data.start);
			fat_remove_chain(clst, 0);
		}
		/* file을 닫을 때 disk_inode의 변경사항을 disk에 write */
		disk_write(filesys_disk, inode->sector, &inode->data);

		free(inode);
	}
}

/* Marks INODE to be deleted when it is closed by the last caller who
 * has it open. */
void inode_remove(struct inode *inode)
{
	ASSERT(inode != NULL);
	inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
 * Returns the number of bytes actually read, which may be less
 * than SIZE if an error occurs or end of file is reached. */
off_t inode_read_at(struct inode *inode, void *buffer_, off_t size, off_t offset)
{
	uint8_t *buffer = buffer_;
	off_t bytes_read = 0;
	uint8_t *bounce = NULL;

	while (size > 0)
	{
		/* Disk sector to read, starting byte offset within sector. */
		disk_sector_t sector_idx = byte_to_sector(inode, offset);
		int sector_ofs = offset % DISK_SECTOR_SIZE;

		/* Bytes left in inode, bytes left in sector, lesser of the two. */
		off_t inode_left = inode_length(inode) - offset;
		int sector_left = DISK_SECTOR_SIZE - sector_ofs;
		int min_left = inode_left < sector_left ? inode_left : sector_left;

		/* Number of bytes to actually copy out of this sector. */
		int chunk_size = size < min_left ? size : min_left;
		if (chunk_size <= 0)
			break;

		if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE)
		{
			/* Read full sector directly into caller's buffer. */
			disk_read(filesys_disk, sector_idx, buffer + bytes_read);
		}
		else
		{
			/* Read sector into bounce buffer, then partially copy
			 * into caller's buffer. */
			if (bounce == NULL)
			{
				bounce = malloc(DISK_SECTOR_SIZE);
				if (bounce == NULL)
					break;
			}
			disk_read(filesys_disk, sector_idx, bounce);
			memcpy(buffer + bytes_read, bounce + sector_ofs, chunk_size);
		}

		/* Advance. */
		size -= chunk_size;
		offset += chunk_size;
		bytes_read += chunk_size;
	}
	free(bounce);

	return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
 * Returns the number of bytes actually written, which may be
 * less than SIZE if end of file is reached or an error occurs.
 * (Normally a write at end of file would extend the inode, but
 * growth is not yet implemented.) */
off_t inode_write_at(struct inode *inode, const void *buffer_, off_t size,
					 off_t offset)
{
	const uint8_t *buffer = buffer_;
	off_t bytes_written = 0;
	uint8_t *bounce = NULL;
	off_t origin_offset = offset;

	if (inode->deny_write_cnt)
		return 0;

	while (size > 0)
	{
		/* Sector to write, starting byte offset within sector. */
		disk_sector_t sector_idx = byte_to_sector(inode, offset);
		int sector_ofs = offset % DISK_SECTOR_SIZE;

		/* Bytes left in inode, bytes left in sector, lesser of the two. */
		/* file growth에 의해 length가 길어질 수 있기 때문에 length에 의한 제한을 없앰 */
		// off_t inode_left = inode_length(inode) - offset;
		int sector_left = DISK_SECTOR_SIZE - sector_ofs;
		int min_left = sector_left;
		// int min_left = inode_left < sector_left ? inode_left : sector_left;

		/* Number of bytes to actually write into this sector. */
		int chunk_size = size < min_left ? size : min_left;
		if (chunk_size <= 0)
			break;

		if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE)
		{
			/* Write full sector directly to disk. */
			disk_write(filesys_disk, sector_idx, buffer + bytes_written);
		}
		else
		{
			/* We need a bounce buffer. */
			if (bounce == NULL)
			{
				bounce = malloc(DISK_SECTOR_SIZE);
				if (bounce == NULL)
					break;
			}

			/* If the sector contains data before or after the chunk
			   we're writing, then we need to read in the sector
			   first.  Otherwise we start with a sector of all zeros. */
			if (sector_ofs > 0 || chunk_size < sector_left)
				disk_read(filesys_disk, sector_idx, bounce);
			else
				memset(bounce, 0, DISK_SECTOR_SIZE);
			memcpy(bounce + sector_ofs, buffer + bytes_written, chunk_size);
			disk_write(filesys_disk, sector_idx, bounce);
		}

		/* Advance. */
		size -= chunk_size;
		offset += chunk_size;
		bytes_written += chunk_size;
	}
	free(bounce);

	/* file growth가 되었을 때 inode의 length를 갱신 */
	if(inode_length(inode) < origin_offset + bytes_written)
		inode->data.length = origin_offset + bytes_written;

	return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void inode_deny_write(struct inode *inode)
{
	inode->deny_write_cnt++;
	ASSERT(inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
 * Must be called once by each inode opener who has called
 * inode_deny_write() on the inode, before closing the inode. */
void inode_allow_write(struct inode *inode)
{
	ASSERT(inode->deny_write_cnt > 0);
	ASSERT(inode->deny_write_cnt <= inode->open_cnt);
	inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t inode_length(const struct inode *inode)
{
	return inode->data.length;
}

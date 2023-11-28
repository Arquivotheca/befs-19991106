/*
 *  linux/fs/befs/debug.c
 *
 * Copyright (C) 1999  Makoto Kato (m_kato@ga2.so-net.ne.jp)
 *
 * debug functions
 */

#include <stdarg.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/befs_fs.h>

#ifdef BEFS_DEBUG


void befs_dump_inode (befs_inode * inode)
{
	int i, j;

	printk (" befs_inode infomation\n");

	printk ("  magic1 %08x\n", inode->magic1);
	printk ("  inode_num %lu, %u %u\n", inode->inode_num.allocation_group,
		inode->inode_num.start, inode->inode_num.len); 
	printk ("  uid %lu\n", inode->uid);
	printk ("  gid %lu\n", inode->gid);
	printk ("  mode %08x\n", inode->mode);
	printk ("  flags %08x\n", inode->flags);
	printk ("  create_time %u\n", inode->create_time >> 8);
	printk ("  last_modified_time %u\n", inode->last_modified_time >> 8);
	printk ("  parent %lu, %u, %u\n", inode->parent.allocation_group,
		inode->parent.start, inode->parent.len);
	printk ("  attributes %lu, %u, %u\n",
		inode->attributes.allocation_group,
		inode->attributes.start, inode->attributes.len);
	printk ("  type %08x\n", inode->type);
	printk ("  inode_size %lu\n", inode->inode_size);

	if (S_ISLNK(inode->mode)) {

		/*
		 * This is symbolic link.
		 */

		printk ("  Symbolic link [%s]\n", inode->data.symlink);
	} else {
		int i;

		for (i = 0; i < BEFS_NUM_DIRECT_BLOCKS; i++) {
			printk ("  direct %d [%lu, %u, %u]\n",
				inode->data.datastream.direct[i].allocation_group,
				inode->data.datastream.direct[i].start,
				inode->data.datastream.direct[i].len);
		}
		printk ("  max_direct_range %Lu\n",
			inode->data.datastream.max_direct_range);
		printk ("  indirect [%lu, %u, %u]\n",
			inode->data.datastream.indirect.allocation_group,
			inode->data.datastream.indirect.start,
			inode->data.datastream.indirect.len);
		printk ("  max_indirect_range %016X\n",
			inode->data.datastream.max_indirect_range);
		printk ("  double indirect [%lu, %u, %u]\n",
			inode->data.datastream.double_indirect.allocation_group,
			inode->data.datastream.double_indirect.start,
			inode->data.datastream.double_indirect.len);
		printk ("  max_double_indirect_range %016X\n",
			inode->data.datastream.max_double_indirect_range);
		printk ("  size %016X\n",
			inode->data.datastream.size);
	}
}


/*
 * Display super block structure for debug.
 */

void befs_dump_super_block (befs_super_block * bs)
{
	int i, j;

	printk (" befs_super_block information\n");

	printk ("  name %s\n", bs->name);
	printk ("  magic1 %08x\n", bs->magic1);
	printk ("  fs_byte_order %08x\n", bs->fs_byte_order);

	printk ("  block_size %lu\n", bs->block_size);
	printk ("  block_shift %lu\n", bs->block_shift);

	printk ("  num_blocks %016X\n", bs->num_blocks);
	printk ("  used_blocks %016X\n", bs->used_blocks);

	printk ("  magic2 %08x\n", bs->magic2);
	printk ("  blocks_per_ag %lu\n", bs->blocks_per_ag);
	printk ("  ag_shift %lu\n", bs->ag_shift);
	printk ("  num_ags %lu\n", bs->num_ags);

	printk ("  flags %08x\n", bs->flags);

	printk ("  log_blocks %lu, %u, %u\n",
		bs->log_blocks.allocation_group,
		bs->log_blocks.start, bs->log_blocks.len);
	printk ("  log_start %Ld\n", bs->log_start);
	printk ("  log_end %Ld\n", bs->log_end);

	printk ("  magic3 %08x\n", bs->magic3);
	printk ("  root_dir %lu, %u, %u\n",
		bs->root_dir.allocation_group, bs->root_dir.start,
		bs->root_dir.len);
	printk ("  indices %lu, %u, %u\n",
		bs->indices.allocation_group, bs->indices.start,
		bs->indices.len);
}



void befs_dump_small_data (befs_small_data * sd)
{
}



void befs_dump_inode_addr (befs_inode_addr iaddr)
{
	printk (" inode addr [%lu, %u, %u]\n",
		iaddr.allocation_group, iaddr.start, iaddr.len);
}



void befs_dump_index_entry (befs_index_entry * entry)
{
	printk (" index entry structure\n");
	printk ("  magic %08x\n", entry->magic);
	printk ("  node_size %lu\n", entry->node_size);
	printk ("  max_number_of_levels %08x\n",
		entry->max_number_of_levels);
	printk ("  data_type %08x\n", entry->data_type);
	printk ("  root_node_pointer %016X\n", entry->root_node_pointer);
	printk ("  free_node_pointer %016X\n", entry->free_node_pointer);
	printk ("  maximum size %016X\n", entry->maximum_size);
}


void befs_dump_index_node (befs_index_node * node)
{
	printk (" inode of index node structure\n");
	printk ("  left %016X\n", node->left);
	printk ("  right %016X\n", node->right);
	printk ("  overflow %016X\n", node->overflow);
	printk ("  all_key_count %u\n", node->all_key_count);
	printk ("  all_key_length %u\n", node->all_key_length);
}

#endif

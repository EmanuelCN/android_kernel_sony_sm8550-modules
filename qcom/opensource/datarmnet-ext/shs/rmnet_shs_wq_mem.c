/* Copyright (c) 2019-2020 The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "rmnet_shs_modules.h"
#include "rmnet_shs_common.h"
#include "rmnet_shs_wq_mem.h"
#include <linux/anon_inodes.h>
#include <linux/file.h>
#include <linux/proc_fs.h>
#include <linux/refcount.h>
MODULE_LICENSE("\x47\x50\x4c\x20\x76\x32");
struct proc_dir_entry *DATARMNETe4c5563cdb;
struct DATARMNET33582f7450 DATARMNET63c47f3c37[DATARMNETc6782fed88];
struct DATARMNETf44cda1bf2 DATARMNET22b4032799[DATARMNET75c69edb82];
struct DATARMNET3a84fbfeae DATARMNETb0d78d576f[DATARMNET75c69edb82];
struct DATARMNETc13d990bf1 DATARMNET3d25ff4ef4[DATARMNET67578af78f];
struct DATARMNET26737fb11e DATARMNET2fdef9da24[DATARMNET67578af78f];
struct DATARMNET57ccbe14f3 DATARMNETf46265286b[DATARMNETe4d15b9332];
struct list_head DATARMNET6c23f11e81 = LIST_HEAD_INIT(DATARMNET6c23f11e81);
struct list_head DATARMNETf91b305f4e = LIST_HEAD_INIT(DATARMNETf91b305f4e);
struct list_head DATARMNETe46ae760db = LIST_HEAD_INIT(DATARMNETe46ae760db);
struct list_head DATARMNET3208cd0982 = LIST_HEAD_INIT(DATARMNET3208cd0982);
struct list_head DATARMNET922b4752e2 = LIST_HEAD_INIT(DATARMNET922b4752e2);
struct DATARMNETa41d4c4a12 *DATARMNET410036d5ac;
struct DATARMNETa41d4c4a12 *DATARMNET19c47a9f3a;
struct DATARMNETa41d4c4a12 *DATARMNET22e796eff3;
struct DATARMNETa41d4c4a12 *DATARMNET9b8000d2a7;
struct DATARMNETa41d4c4a12 *DATARMNET835a28686c;
struct DATARMNETa41d4c4a12 *DATARMNET67d31dc40a;

struct rmnet_shs_open_file {
	struct DATARMNETa41d4c4a12 *buffer;
	/* Weak reference protected by the SHS lock. */
	struct file *mapping_file;
};

/*
 * Procfs can release an open file during entry removal. Read its private
 * data under the same lock that clears it and pin the page before unlocking.
 */
static vm_fault_t rmnet_shs_mapping_fault(struct vm_fault *vmf)
{
	struct file *proc_file = vmf->vma->vm_file->private_data;
	struct rmnet_shs_open_file *open_file;
	struct DATARMNETa41d4c4a12 *info;
	struct page *page;

	DATARMNET6bf538fa23();
	open_file = proc_file->private_data;
	info = open_file ? open_file->buffer : NULL;
	if (!info || !info->data) {
		DATARMNETaea4c85748();
		return VM_FAULT_SIGSEGV;
	}

	page = virt_to_page(info->data);
	get_page(page);
	vmf->page = page;
	DATARMNETaea4c85748();
	return 0;
}

static const struct vm_operations_struct rmnet_shs_vm_ops = {
	.fault = rmnet_shs_mapping_fault,
};

static int rmnet_shs_mapping_remap(struct file *file,
				   struct vm_area_struct *vma)
{
	/* Go through procfs so remapping checks for entry removal too. */
	return call_mmap(file->private_data, vma);
}

static int rmnet_shs_mapping_release(struct inode *inode, struct file *file)
{
	struct file *proc_file = file->private_data;
	struct rmnet_shs_open_file *open_file;

	DATARMNET6bf538fa23();
	open_file = proc_file->private_data;
	if (open_file && open_file->mapping_file == file)
		open_file->mapping_file = NULL;
	DATARMNETaea4c85748();

	fput(proc_file);
	return 0;
}

static const struct file_operations rmnet_shs_mapping_fops = {
	.owner = THIS_MODULE,
	.mmap = rmnet_shs_mapping_remap,
	.release = rmnet_shs_mapping_release,
};

/*
 * Take a reference to the live backing file, or install the caller's new
 * file. The release callback removes the weak reference before the file
 * can be freed, and get_file_rcu() prevents reviving a file being released.
 */
static struct file *
rmnet_shs_get_mapping_file(struct rmnet_shs_open_file *open_file,
			   struct file *new_file)
{
	struct file *mapping_file;

	DATARMNET6bf538fa23();
	mapping_file = open_file->mapping_file;
	if (!mapping_file || !get_file_rcu(mapping_file)) {
		mapping_file = new_file;
		open_file->mapping_file = new_file;
	}
	DATARMNETaea4c85748();

	return mapping_file;
}

static int rmnet_shs_proc_mmap(struct file *file, struct vm_area_struct *vma)
{
	/* Procfs keeps the open state alive while this callback runs. */
	struct rmnet_shs_open_file *open_file = file->private_data;
	struct file *mapping_file;

	/* Remapping through the anonymous file can bypass procfs's noexec. */
	if (vma->vm_flags & VM_EXEC)
		return -EPERM;

	mapping_file = rmnet_shs_get_mapping_file(open_file, NULL);
	if (!mapping_file) {
		struct file *new_file;

		/* Keep both the module and the proc file alive across unmap. */
		new_file = anon_inode_getfile("[rmnet_shs]",
					      &rmnet_shs_mapping_fops, file,
					      file->f_flags);
		if (IS_ERR(new_file))
			return PTR_ERR(new_file);

		get_file(file);
		mapping_file = rmnet_shs_get_mapping_file(open_file, new_file);
		if (mapping_file != new_file)
			fput(new_file);
	}

	vm_flags_mod(vma, VM_DONTEXPAND | VM_DONTDUMP, VM_MAYEXEC);
	vma->vm_ops = &rmnet_shs_vm_ops;
	swap(vma->vm_file, mapping_file);
	fput(mapping_file);
	return 0;
}

static void rmnet_shs_release_buffer(struct file *file,
				     struct DATARMNETa41d4c4a12 **shared)
{
	struct rmnet_shs_open_file *open_file;
	struct DATARMNETa41d4c4a12 *info;

	DATARMNET6bf538fa23();
	open_file = file->private_data;
	file->private_data = NULL;
	info = open_file ? open_file->buffer : NULL;
	if (info && refcount_dec_and_test(&info->refcnt)) {
		if (*shared == info)
			*shared = NULL;
		free_page((unsigned long)info->data);
		kfree(info);
	}
	DATARMNETaea4c85748();
	kfree(open_file);
}

static int DATARMNET33a80d3bf7(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	struct DATARMNETa41d4c4a12 *DATARMNET54338da2ff;
	struct rmnet_shs_open_file *open_file;

	open_file = kzalloc(sizeof(*open_file), GFP_KERNEL);
	if (!open_file)
		return -ENOMEM;

	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	DATARMNET6bf538fa23();
	if (!DATARMNET410036d5ac) {
		DATARMNET54338da2ff =
			kzalloc(sizeof(struct DATARMNETa41d4c4a12), GFP_ATOMIC);
		if (!DATARMNET54338da2ff)
			goto DATARMNET1d7e728ddf;
		DATARMNET54338da2ff->data = (char *)get_zeroed_page(GFP_ATOMIC);
		if (!DATARMNET54338da2ff->data) {
			kfree(DATARMNET54338da2ff);
			goto DATARMNET1d7e728ddf;
		}
		DATARMNET410036d5ac = DATARMNET54338da2ff;
		refcount_set(&DATARMNET410036d5ac->refcnt,
			     (0xd26 + 209 - 0xdf6));
		rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x76\x69\x72\x74\x5f\x74\x6f\x5f\x70\x68\x79\x73\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x63\x61\x70\x5f\x73\x68\x61\x72\x65\x64\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78"
		       "\n",
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET54338da2ff),
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET410036d5ac));
	} else {
		refcount_inc(&DATARMNET410036d5ac->refcnt);
	}
	open_file->buffer = DATARMNET410036d5ac;
	DATARMNET3cc114dce0->private_data = open_file;
	DATARMNETaea4c85748();
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x2d\x20\x4f\x4b"
	       "\n");
	return (0xd2d + 202 - 0xdf7);
DATARMNET1d7e728ddf:
	DATARMNETaea4c85748();
	kfree(open_file);
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x2d\x20\x46\x41\x49\x4c\x45\x44"
	       "\n");
	return -ENOMEM;
}
static int DATARMNET0935e3beb9(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	struct DATARMNETa41d4c4a12 *DATARMNET54338da2ff;
	struct rmnet_shs_open_file *open_file;

	open_file = kzalloc(sizeof(*open_file), GFP_KERNEL);
	if (!open_file)
		return -ENOMEM;

	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x67\x5f\x66\x6c\x6f\x77\x73\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	DATARMNET6bf538fa23();
	if (!DATARMNET19c47a9f3a) {
		DATARMNET54338da2ff =
			kzalloc(sizeof(struct DATARMNETa41d4c4a12), GFP_ATOMIC);
		if (!DATARMNET54338da2ff)
			goto DATARMNET1d7e728ddf;
		DATARMNET54338da2ff->data = (char *)get_zeroed_page(GFP_ATOMIC);
		if (!DATARMNET54338da2ff->data) {
			kfree(DATARMNET54338da2ff);
			goto DATARMNET1d7e728ddf;
		}
		DATARMNET19c47a9f3a = DATARMNET54338da2ff;
		refcount_set(&DATARMNET19c47a9f3a->refcnt,
			     (0xd26 + 209 - 0xdf6));
		rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x76\x69\x72\x74\x5f\x74\x6f\x5f\x70\x68\x79\x73\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x67\x66\x6c\x6f\x77\x5f\x73\x68\x61\x72\x65\x64\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78"
		       "\n",
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET54338da2ff),
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET19c47a9f3a));
	} else {
		refcount_inc(&DATARMNET19c47a9f3a->refcnt);
	}
	open_file->buffer = DATARMNET19c47a9f3a;
	DATARMNET3cc114dce0->private_data = open_file;
	DATARMNETaea4c85748();
	return (0xd2d + 202 - 0xdf7);
DATARMNET1d7e728ddf:
	DATARMNETaea4c85748();
	kfree(open_file);
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x2d\x20\x46\x41\x49\x4c\x45\x44"
	       "\n");
	return -ENOMEM;
}
static int DATARMNETfbd34e70a2(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	struct DATARMNETa41d4c4a12 *DATARMNET54338da2ff;
	struct rmnet_shs_open_file *open_file;

	open_file = kzalloc(sizeof(*open_file), GFP_KERNEL);
	if (!open_file)
		return -ENOMEM;

	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x73\x73\x5f\x66\x6c\x6f\x77\x73\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	DATARMNET6bf538fa23();
	if (!DATARMNET22e796eff3) {
		DATARMNET54338da2ff =
			kzalloc(sizeof(struct DATARMNETa41d4c4a12), GFP_ATOMIC);
		if (!DATARMNET54338da2ff)
			goto DATARMNET1d7e728ddf;
		DATARMNET54338da2ff->data = (char *)get_zeroed_page(GFP_ATOMIC);
		if (!DATARMNET54338da2ff->data) {
			kfree(DATARMNET54338da2ff);
			goto DATARMNET1d7e728ddf;
		}
		DATARMNET22e796eff3 = DATARMNET54338da2ff;
		refcount_set(&DATARMNET22e796eff3->refcnt,
			     (0xd26 + 209 - 0xdf6));
		rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x76\x69\x72\x74\x5f\x74\x6f\x5f\x70\x68\x79\x73\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x73\x73\x66\x6c\x6f\x77\x5f\x73\x68\x61\x72\x65\x64\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78"
		       "\n",
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET54338da2ff),
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET22e796eff3));
	} else {
		refcount_inc(&DATARMNET22e796eff3->refcnt);
	}
	open_file->buffer = DATARMNET22e796eff3;
	DATARMNET3cc114dce0->private_data = open_file;
	DATARMNETaea4c85748();
	return (0xd2d + 202 - 0xdf7);
DATARMNET1d7e728ddf:
	DATARMNETaea4c85748();
	kfree(open_file);
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x2d\x20\x46\x41\x49\x4c\x45\x44"
	       "\n");
	return -ENOMEM;
}
static int DATARMNET57bcb932f3(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	struct DATARMNETa41d4c4a12 *DATARMNET54338da2ff;
	struct rmnet_shs_open_file *open_file;

	open_file = kzalloc(sizeof(*open_file), GFP_KERNEL);
	if (!open_file)
		return -ENOMEM;

	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x66\x66\x6c\x6f\x77\x73\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	DATARMNET6bf538fa23();
	if (!DATARMNET9b8000d2a7) {
		DATARMNET54338da2ff =
			kzalloc(sizeof(struct DATARMNETa41d4c4a12), GFP_ATOMIC);
		if (!DATARMNET54338da2ff)
			goto DATARMNET1d7e728ddf;
		DATARMNET54338da2ff->data = (char *)get_zeroed_page(GFP_ATOMIC);
		if (!DATARMNET54338da2ff->data) {
			kfree(DATARMNET54338da2ff);
			goto DATARMNET1d7e728ddf;
		}
		DATARMNET9b8000d2a7 = DATARMNET54338da2ff;
		refcount_set(&DATARMNET9b8000d2a7->refcnt,
			     (0xd26 + 209 - 0xdf6));
		rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x76\x69\x72\x74\x5f\x74\x6f\x5f\x70\x68\x79\x73\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x66\x66\x6c\x6f\x77\x5f\x73\x68\x61\x72\x65\x64\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78"
		       "\n",
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET54338da2ff),
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET9b8000d2a7));
	} else {
		refcount_inc(&DATARMNET9b8000d2a7->refcnt);
	}
	open_file->buffer = DATARMNET9b8000d2a7;
	DATARMNET3cc114dce0->private_data = open_file;
	DATARMNETaea4c85748();
	return (0xd2d + 202 - 0xdf7);
DATARMNET1d7e728ddf:
	DATARMNETaea4c85748();
	kfree(open_file);
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x2d\x20\x46\x41\x49\x4c\x45\x44"
	       "\n");
	return -ENOMEM;
}
static int DATARMNET601894f27b(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	struct DATARMNETa41d4c4a12 *DATARMNET54338da2ff;
	struct rmnet_shs_open_file *open_file;

	open_file = kzalloc(sizeof(*open_file), GFP_KERNEL);
	if (!open_file)
		return -ENOMEM;

	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x6c\x6c\x5f\x66\x6c\x6f\x77\x73\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	DATARMNET6bf538fa23();
	if (!DATARMNET835a28686c) {
		DATARMNET54338da2ff =
			kzalloc(sizeof(struct DATARMNETa41d4c4a12), GFP_ATOMIC);
		if (!DATARMNET54338da2ff)
			goto DATARMNET1d7e728ddf;
		DATARMNET54338da2ff->data = (char *)get_zeroed_page(GFP_ATOMIC);
		if (!DATARMNET54338da2ff->data) {
			kfree(DATARMNET54338da2ff);
			goto DATARMNET1d7e728ddf;
		}
		DATARMNET835a28686c = DATARMNET54338da2ff;
		refcount_set(&DATARMNET835a28686c->refcnt,
			     (0xd26 + 209 - 0xdf6));
		rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x76\x69\x72\x74\x5f\x74\x6f\x5f\x70\x68\x79\x73\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x6c\x6c\x66\x6c\x6f\x77\x5f\x73\x68\x61\x72\x65\x64\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78"
		       "\n",
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET54338da2ff),
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET835a28686c));
	} else {
		refcount_inc(&DATARMNET835a28686c->refcnt);
	}
	open_file->buffer = DATARMNET835a28686c;
	DATARMNET3cc114dce0->private_data = open_file;
	DATARMNETaea4c85748();
	return (0xd2d + 202 - 0xdf7);
DATARMNET1d7e728ddf:
	DATARMNETaea4c85748();
	kfree(open_file);
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x2d\x20\x46\x41\x49\x4c\x45\x44"
	       "\n");
	return -ENOMEM;
}
static int DATARMNETb9d10d8cc0(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	struct DATARMNETa41d4c4a12 *DATARMNET54338da2ff;
	struct rmnet_shs_open_file *open_file;

	open_file = kzalloc(sizeof(*open_file), GFP_KERNEL);
	if (!open_file)
		return -ENOMEM;

	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x6f\x70\x65\x6e\x20\x6e\x65\x74\x64\x65\x76\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	DATARMNET6bf538fa23();
	if (!DATARMNET67d31dc40a) {
		DATARMNET54338da2ff =
			kzalloc(sizeof(struct DATARMNETa41d4c4a12), GFP_ATOMIC);
		if (!DATARMNET54338da2ff)
			goto DATARMNET1d7e728ddf;
		DATARMNET54338da2ff->data = (char *)get_zeroed_page(GFP_ATOMIC);
		if (!DATARMNET54338da2ff->data) {
			kfree(DATARMNET54338da2ff);
			goto DATARMNET1d7e728ddf;
		}
		DATARMNET67d31dc40a = DATARMNET54338da2ff;
		refcount_set(&DATARMNET67d31dc40a->refcnt,
			     (0xd26 + 209 - 0xdf6));
		rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x76\x69\x72\x74\x5f\x74\x6f\x5f\x70\x68\x79\x73\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x6e\x65\x74\x64\x65\x76\x5f\x73\x68\x61\x72\x65\x64\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78"
		       "\n",
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET54338da2ff),
		       (unsigned long long)virt_to_phys(
			       (void *)DATARMNET67d31dc40a));
	} else {
		refcount_inc(&DATARMNET67d31dc40a->refcnt);
	}
	open_file->buffer = DATARMNET67d31dc40a;
	DATARMNET3cc114dce0->private_data = open_file;
	DATARMNETaea4c85748();
	return (0xd2d + 202 - 0xdf7);
DATARMNET1d7e728ddf:
	DATARMNETaea4c85748();
	kfree(open_file);
	return -ENOMEM;
}
static ssize_t DATARMNETaf2aa1d70f(struct file *DATARMNET3cc114dce0,
				   char __user *buf, size_t len,
				   loff_t *DATARMNET6396f657b3)
{
	return DATARMNET29d29f44cf;
}
static ssize_t DATARMNET4761dcd3fd(struct file *DATARMNET3cc114dce0,
				   const char __user *buf, size_t len,
				   loff_t *DATARMNET6396f657b3)
{
	return len;
}
static int DATARMNETcfbc4f0c39(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x72\x65\x6c\x65\x61\x73\x65\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	rmnet_shs_release_buffer(DATARMNET3cc114dce0, &DATARMNET410036d5ac);
	return (0xd2d + 202 - 0xdf7);
}
static int DATARMNET2a9f67714f(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x72\x65\x6c\x65\x61\x73\x65\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	rmnet_shs_release_buffer(DATARMNET3cc114dce0, &DATARMNET19c47a9f3a);
	return (0xd2d + 202 - 0xdf7);
}
static int DATARMNET968bc5b80d(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x72\x65\x6c\x65\x61\x73\x65\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	rmnet_shs_release_buffer(DATARMNET3cc114dce0, &DATARMNET22e796eff3);
	return (0xd2d + 202 - 0xdf7);
}
static int DATARMNET59b415aded(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x72\x65\x6c\x65\x61\x73\x65\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	rmnet_shs_release_buffer(DATARMNET3cc114dce0, &DATARMNET9b8000d2a7);
	return (0xd2d + 202 - 0xdf7);
}
static int DATARMNET794d7587bd(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x72\x65\x6c\x65\x61\x73\x65\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	rmnet_shs_release_buffer(DATARMNET3cc114dce0, &DATARMNET835a28686c);
	return (0xd2d + 202 - 0xdf7);
}
static int DATARMNET18464ea5e3(struct inode *inode,
			       struct file *DATARMNET3cc114dce0)
{
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x72\x6d\x6e\x65\x74\x5f\x73\x68\x73\x5f\x72\x65\x6c\x65\x61\x73\x65\x20\x6e\x65\x74\x64\x65\x76\x20\x2d\x20\x65\x6e\x74\x72\x79"
	       "\n");
	rmnet_shs_release_buffer(DATARMNET3cc114dce0, &DATARMNET67d31dc40a);
	return (0xd2d + 202 - 0xdf7);
}
static const struct proc_ops DATARMNET668485bd14 = {
	.proc_mmap = rmnet_shs_proc_mmap,
	.proc_open = DATARMNET33a80d3bf7,
	.proc_release = DATARMNETcfbc4f0c39,
	.proc_read = DATARMNETaf2aa1d70f,
	.proc_write = DATARMNET4761dcd3fd,
};
static const struct proc_ops DATARMNETe84fbaa6af = {
	.proc_mmap = rmnet_shs_proc_mmap,
	.proc_open = DATARMNET0935e3beb9,
	.proc_release = DATARMNET2a9f67714f,
	.proc_read = DATARMNETaf2aa1d70f,
	.proc_write = DATARMNET4761dcd3fd,
};
static const struct proc_ops DATARMNET8fe5f892a8 = {
	.proc_mmap = rmnet_shs_proc_mmap,
	.proc_open = DATARMNETfbd34e70a2,
	.proc_release = DATARMNET968bc5b80d,
	.proc_read = DATARMNETaf2aa1d70f,
	.proc_write = DATARMNET4761dcd3fd,
};
static const struct proc_ops DATARMNET0104d40d4b = {
	.proc_mmap = rmnet_shs_proc_mmap,
	.proc_open = DATARMNET57bcb932f3,
	.proc_release = DATARMNET59b415aded,
	.proc_read = DATARMNETaf2aa1d70f,
	.proc_write = DATARMNET4761dcd3fd,
};
static const struct proc_ops DATARMNETddcdf7bd4e = {
	.proc_mmap = rmnet_shs_proc_mmap,
	.proc_open = DATARMNET601894f27b,
	.proc_release = DATARMNET794d7587bd,
	.proc_read = DATARMNETaf2aa1d70f,
	.proc_write = DATARMNET4761dcd3fd,
};
static const struct proc_ops DATARMNET6eb63d9ad0 = {
	.proc_mmap = rmnet_shs_proc_mmap,
	.proc_open = DATARMNETb9d10d8cc0,
	.proc_release = DATARMNET18464ea5e3,
	.proc_read = DATARMNETaf2aa1d70f,
	.proc_write = DATARMNET4761dcd3fd,
};
void DATARMNET28a80d526e(struct DATARMNET6c78e47d24 *DATARMNETd2a694d52a,
			 struct list_head *DATARMNETf0fb155a9c)
{
	struct DATARMNETa52c09a590 *DATARMNET0f551e8a47;
	if (!DATARMNETd2a694d52a || !DATARMNETf0fb155a9c) {
		DATARMNET68d84e7b98[DATARMNETac729c3d29]++;
		return;
	}
	DATARMNET0f551e8a47 = kzalloc(sizeof(*DATARMNET0f551e8a47), GFP_ATOMIC);
	if (DATARMNET0f551e8a47 != NULL) {
		DATARMNET0f551e8a47->DATARMNET253a9fc708 =
			DATARMNETd2a694d52a->DATARMNET253a9fc708;
		DATARMNET0f551e8a47->DATARMNET42a992465f =
			DATARMNETd2a694d52a->DATARMNET7c894c2f8f;
		DATARMNET0f551e8a47->hash = DATARMNETd2a694d52a->hash;
		DATARMNET0f551e8a47->DATARMNET324c1a8f98 =
			DATARMNETd2a694d52a->DATARMNET324c1a8f98;
		DATARMNET0f551e8a47->DATARMNETbb80fccd97 =
			DATARMNETd2a694d52a->DATARMNETbb80fccd97;
		list_add(&DATARMNET0f551e8a47->DATARMNET6f898987df,
			 DATARMNETf0fb155a9c);
	} else {
		DATARMNET68d84e7b98[DATARMNET6533f70c87]++;
	}
}
void DATARMNET3af54cd726(struct list_head *DATARMNETf0fb155a9c)
{
	struct DATARMNETa52c09a590 *DATARMNET0f551e8a47;
	struct list_head *DATARMNET7b34b7b5be, *next;
	if (!DATARMNETf0fb155a9c) {
		DATARMNET68d84e7b98[DATARMNETac729c3d29]++;
		return;
	}
	list_for_each_safe (DATARMNET7b34b7b5be, next, DATARMNETf0fb155a9c) {
		DATARMNET0f551e8a47 = list_entry(DATARMNET7b34b7b5be,
						 struct DATARMNETa52c09a590,
						 DATARMNET6f898987df);
		list_del_init(&DATARMNET0f551e8a47->DATARMNET6f898987df);
		kfree(DATARMNET0f551e8a47);
	}
}
void DATARMNET24e4475345(struct DATARMNET6c78e47d24 *DATARMNETd2a694d52a,
			 struct list_head *DATARMNETb436c3f30b)
{
	struct DATARMNET09a412b1c6 *DATARMNET4238158b2a;
	if (!DATARMNETd2a694d52a || !DATARMNETb436c3f30b) {
		DATARMNET68d84e7b98[DATARMNETac729c3d29]++;
		return;
	}
	if (!DATARMNET362b15f941(DATARMNETd2a694d52a->DATARMNET7c894c2f8f)) {
		DATARMNET4238158b2a =
			kzalloc(sizeof(*DATARMNET4238158b2a), GFP_ATOMIC);
		if (DATARMNET4238158b2a != NULL) {
			DATARMNET4238158b2a->DATARMNET253a9fc708 =
				DATARMNETd2a694d52a->DATARMNET253a9fc708;
			DATARMNET4238158b2a->DATARMNET42a992465f =
				DATARMNETd2a694d52a->DATARMNET7c894c2f8f;
			DATARMNET4238158b2a->hash = DATARMNETd2a694d52a->hash;
			DATARMNET4238158b2a->DATARMNET324c1a8f98 =
				DATARMNETd2a694d52a->DATARMNET324c1a8f98;
			list_add(&DATARMNET4238158b2a->DATARMNETd502c0412a,
				 DATARMNETb436c3f30b);
		} else {
			DATARMNET68d84e7b98[DATARMNET6533f70c87]++;
		}
	}
}
void DATARMNETa1b141715b(struct list_head *DATARMNETb436c3f30b)
{
	struct DATARMNET09a412b1c6 *DATARMNET4238158b2a;
	struct list_head *DATARMNET7b34b7b5be, *next;
	if (!DATARMNETb436c3f30b) {
		DATARMNET68d84e7b98[DATARMNETac729c3d29]++;
		return;
	}
	list_for_each_safe (DATARMNET7b34b7b5be, next, DATARMNETb436c3f30b) {
		DATARMNET4238158b2a = list_entry(DATARMNET7b34b7b5be,
						 struct DATARMNET09a412b1c6,
						 DATARMNETd502c0412a);
		list_del_init(&DATARMNET4238158b2a->DATARMNETd502c0412a);
		kfree(DATARMNET4238158b2a);
	}
}
void DATARMNETc6439e3d71(struct DATARMNETc8fdbf9c85 *DATARMNET7bea4a06a6,
			 struct DATARMNET228056d4b7 *DATARMNETf632b170b1,
			 struct list_head *DATARMNETe46ae760db)
{
	u64 DATARMNET264b01f4d5, DATARMNET53ce143c7e = (0xd2d + 202 - 0xdf7);
	struct DATARMNET47a6995138 *DATARMNETace28a2c7f;
	int flows = (0xd2d + 202 - 0xdf7);
	if (!DATARMNETf632b170b1 || !DATARMNETe46ae760db) {
		DATARMNET68d84e7b98[DATARMNETac729c3d29]++;
		return;
	}
	flows = DATARMNET7bea4a06a6
			->DATARMNET73464778dc[DATARMNETf632b170b1
						      ->DATARMNET42a992465f]
			.flows;
	DATARMNET264b01f4d5 =
		DATARMNET713717107f[DATARMNETf632b170b1->DATARMNET42a992465f];
	DATARMNET53ce143c7e =
		DATARMNET4793ed48af[DATARMNETf632b170b1->DATARMNET42a992465f];
	DATARMNETace28a2c7f = kzalloc(sizeof(*DATARMNETace28a2c7f), GFP_ATOMIC);
	if (DATARMNETace28a2c7f == NULL) {
		DATARMNET68d84e7b98[DATARMNET6533f70c87]++;
		return;
	}
	DATARMNETace28a2c7f->DATARMNET42a992465f =
		DATARMNETf632b170b1->DATARMNET42a992465f;
	if (flows <= (0xd2d + 202 - 0xdf7)) {
		DATARMNETace28a2c7f->DATARMNET18b7a5b761 = DATARMNET264b01f4d5;
		DATARMNETace28a2c7f->DATARMNET4da6031170 = DATARMNET264b01f4d5;
		DATARMNETace28a2c7f->DATARMNET4df302dbd6 =
			(0xd2d + 202 - 0xdf7);
		list_add(&DATARMNETace28a2c7f->DATARMNET02523bfb57,
			 DATARMNETe46ae760db);
		return;
	}
	if (DATARMNETf632b170b1->DATARMNET324c1a8f98 < DATARMNET264b01f4d5) {
		DATARMNETace28a2c7f->DATARMNET18b7a5b761 =
			DATARMNET264b01f4d5 -
			DATARMNETf632b170b1->DATARMNET324c1a8f98;
	} else {
		DATARMNETace28a2c7f->DATARMNET18b7a5b761 =
			(0xd2d + 202 - 0xdf7);
	}
	if (DATARMNETf632b170b1->DATARMNET253a9fc708 < DATARMNET264b01f4d5) {
		DATARMNETace28a2c7f->DATARMNET4da6031170 =
			DATARMNET264b01f4d5 -
			DATARMNETf632b170b1->DATARMNET253a9fc708;
	} else {
		DATARMNETace28a2c7f->DATARMNET4da6031170 =
			(0xd2d + 202 - 0xdf7);
	}
	DATARMNETace28a2c7f->DATARMNET4df302dbd6 =
		DATARMNETf632b170b1->DATARMNETbb80fccd97;
	list_add(&DATARMNETace28a2c7f->DATARMNET02523bfb57,
		 DATARMNETe46ae760db);
}
void DATARMNETb177316a15(struct list_head *DATARMNETe46ae760db)
{
	struct DATARMNET47a6995138 *DATARMNETace28a2c7f;
	struct list_head *DATARMNET7b34b7b5be, *next;
	if (!DATARMNETe46ae760db) {
		DATARMNET68d84e7b98[DATARMNETac729c3d29]++;
		return;
	}
	list_for_each_safe (DATARMNET7b34b7b5be, next, DATARMNETe46ae760db) {
		DATARMNETace28a2c7f = list_entry(DATARMNET7b34b7b5be,
						 struct DATARMNET47a6995138,
						 DATARMNET02523bfb57);
		list_del_init(&DATARMNETace28a2c7f->DATARMNET02523bfb57);
		kfree(DATARMNETace28a2c7f);
	}
}
void DATARMNET312b06829d(struct DATARMNET6c78e47d24 *DATARMNETd2a694d52a,
			 struct list_head *DATARMNET922b4752e2)
{
	struct DATARMNETe176be7f7c *DATARMNET4af6567922;
	if (!DATARMNETd2a694d52a || !DATARMNET922b4752e2) {
		DATARMNET68d84e7b98[DATARMNETac729c3d29]++;
		return;
	}
	DATARMNET4af6567922 = kzalloc(sizeof(*DATARMNET4af6567922), GFP_ATOMIC);
	if (DATARMNET4af6567922 != NULL &&
	    DATARMNETd2a694d52a->DATARMNET63b1a086d5 != NULL) {
		DATARMNET4af6567922->DATARMNET253a9fc708 =
			DATARMNETd2a694d52a->DATARMNET253a9fc708;
		DATARMNET4af6567922->DATARMNET42a992465f =
			DATARMNETd2a694d52a->DATARMNET7c894c2f8f;
		DATARMNET4af6567922->hash = DATARMNETd2a694d52a->hash;
		DATARMNET4af6567922->DATARMNET324c1a8f98 =
			DATARMNETd2a694d52a->DATARMNET324c1a8f98;
		DATARMNET4af6567922->DATARMNETbb80fccd97 =
			DATARMNETd2a694d52a->DATARMNETbb80fccd97;
		DATARMNET4af6567922->trans_proto =
			DATARMNETd2a694d52a->DATARMNET1e9d25d9ff;
		DATARMNET4af6567922->mux_id = DATARMNETd2a694d52a->mux_id;
		DATARMNET4af6567922->DATARMNET95266642d1 =
			DATARMNETd2a694d52a->DATARMNET95266642d1;
		DATARMNET4af6567922->DATARMNET0d682bcb29 =
			DATARMNETd2a694d52a->DATARMNET0d682bcb29;
		DATARMNET4af6567922->DATARMNET20e8fc9db8 =
			DATARMNETd2a694d52a->DATARMNET20e8fc9db8;
		DATARMNET4af6567922->DATARMNETda8e8ae7ed =
			DATARMNETd2a694d52a->DATARMNET29c6349349;
		DATARMNETd2da2e8466(
			"\x53\x48\x53\x5f\x48\x57\x5f\x43\x4f\x41\x4c\x5f\x4d\x45\x4d\x3a\x20\x68\x77\x20\x63\x6f\x61\x6c\x20\x62\x79\x74\x65\x73\x20\x64\x69\x66\x66\x20\x3d\x20\x25\x6c\x75\x20\x68\x77\x20\x63\x6f\x61\x6c\x20\x62\x75\x66\x73\x69\x7a\x65\x20\x64\x69\x66\x66\x20\x3d\x20\x25\x6c\x75",
			DATARMNET4af6567922->DATARMNET0d682bcb29,
			DATARMNET4af6567922->DATARMNET20e8fc9db8);
		rm_err("\x53\x48\x53\x5f\x46\x4d\x55\x58\x3a\x20\x6d\x75\x78\x20\x69\x64\x20\x66\x6f\x72\x20\x4c\x4c\x20\x68\x61\x73\x68\x20\x30\x78\x25\x78\x20\x69\x73\x20\x25\x64\x20\x74\x70\x20\x25\x64",
		       DATARMNET4af6567922->hash, DATARMNET4af6567922->mux_id,
		       DATARMNET4af6567922->trans_proto);
		memcpy(&DATARMNET4af6567922->ip_hdr,
		       &DATARMNETd2a694d52a->DATARMNET63b1a086d5->ip_hdr,
		       sizeof(DATARMNET4af6567922->ip_hdr));
		memcpy(&DATARMNET4af6567922->DATARMNETe33b41dad9,
		       &DATARMNETd2a694d52a->DATARMNET63b1a086d5
				->DATARMNETe33b41dad9,
		       sizeof(DATARMNET4af6567922->DATARMNETe33b41dad9));
		list_add(&DATARMNET4af6567922->DATARMNET7287946e7b,
			 DATARMNET922b4752e2);
	} else {
		DATARMNET68d84e7b98[DATARMNET6533f70c87]++;
	}
}
void DATARMNET6f4b0915d3(struct DATARMNET6c78e47d24 *DATARMNETd2a694d52a,
			 struct list_head *DATARMNET3208cd0982)
{
	struct DATARMNET13c47d154e *DATARMNETf02b2a5be7;
	if (!DATARMNETd2a694d52a || !DATARMNET3208cd0982) {
		DATARMNET68d84e7b98[DATARMNETac729c3d29]++;
		return;
	}
	DATARMNETf02b2a5be7 = kzalloc(sizeof(*DATARMNETf02b2a5be7), GFP_ATOMIC);
	if (DATARMNETf02b2a5be7 != NULL &&
	    DATARMNETd2a694d52a->DATARMNET63b1a086d5 != NULL) {
		DATARMNETf02b2a5be7->DATARMNET253a9fc708 =
			DATARMNETd2a694d52a->DATARMNET253a9fc708;
		DATARMNETf02b2a5be7->DATARMNET42a992465f =
			DATARMNETd2a694d52a->DATARMNET7c894c2f8f;
		DATARMNETf02b2a5be7->hash = DATARMNETd2a694d52a->hash;
		DATARMNETf02b2a5be7->DATARMNET324c1a8f98 =
			DATARMNETd2a694d52a->DATARMNET324c1a8f98;
		DATARMNETf02b2a5be7->DATARMNETbb80fccd97 =
			DATARMNETd2a694d52a->DATARMNETbb80fccd97;
		DATARMNETf02b2a5be7->trans_proto =
			DATARMNETd2a694d52a->DATARMNET1e9d25d9ff;
		DATARMNETf02b2a5be7->mux_id = DATARMNETd2a694d52a->mux_id;
		DATARMNETf02b2a5be7->DATARMNET95266642d1 =
			DATARMNETd2a694d52a->DATARMNET95266642d1;
		DATARMNETf02b2a5be7->DATARMNET0d682bcb29 =
			DATARMNETd2a694d52a->DATARMNET0d682bcb29;
		DATARMNETf02b2a5be7->DATARMNET20e8fc9db8 =
			DATARMNETd2a694d52a->DATARMNET20e8fc9db8;
		DATARMNETd2da2e8466(
			"\x53\x48\x53\x5f\x48\x57\x5f\x43\x4f\x41\x4c\x5f\x4d\x45\x4d\x3a\x20\x68\x77\x20\x63\x6f\x61\x6c\x20\x62\x79\x74\x65\x73\x20\x64\x69\x66\x66\x20\x3d\x20\x25\x6c\x75\x20\x68\x77\x20\x63\x6f\x61\x6c\x20\x62\x75\x66\x73\x69\x7a\x65\x20\x64\x69\x66\x66\x20\x3d\x20\x25\x6c\x75",
			DATARMNETf02b2a5be7->DATARMNET0d682bcb29,
			DATARMNETf02b2a5be7->DATARMNET20e8fc9db8);
		rm_err("\x53\x48\x53\x5f\x46\x4d\x55\x58\x3a\x20\x6d\x75\x78\x20\x69\x64\x20\x66\x6f\x72\x20\x66\x66\x20\x68\x61\x73\x68\x20\x30\x78\x25\x78\x20\x69\x73\x20\x25\x64\x20\x74\x70\x20\x25\x64",
		       DATARMNETf02b2a5be7->hash, DATARMNETf02b2a5be7->mux_id,
		       DATARMNETf02b2a5be7->trans_proto);
		memcpy(&DATARMNETf02b2a5be7->ip_hdr,
		       &DATARMNETd2a694d52a->DATARMNET63b1a086d5->ip_hdr,
		       sizeof(DATARMNETf02b2a5be7->ip_hdr));
		memcpy(&DATARMNETf02b2a5be7->DATARMNETe33b41dad9,
		       &DATARMNETd2a694d52a->DATARMNET63b1a086d5
				->DATARMNETe33b41dad9,
		       sizeof(DATARMNETf02b2a5be7->DATARMNETe33b41dad9));
		list_add(&DATARMNETf02b2a5be7->DATARMNETec0e3cb8f0,
			 DATARMNET3208cd0982);
	} else {
		DATARMNET68d84e7b98[DATARMNET6533f70c87]++;
	}
}
void DATARMNETf7730d41c1(struct list_head *DATARMNET922b4752e2)
{
	struct DATARMNET13c47d154e *DATARMNET4af6567922;
	struct list_head *DATARMNET7b34b7b5be, *next;
	if (!DATARMNET922b4752e2) {
		DATARMNET68d84e7b98[DATARMNETac729c3d29]++;
		return;
	}
	list_for_each_safe (DATARMNET7b34b7b5be, next, DATARMNET922b4752e2) {
		DATARMNET4af6567922 = list_entry(DATARMNET7b34b7b5be,
						 struct DATARMNET13c47d154e,
						 DATARMNETec0e3cb8f0);
		list_del_init(&DATARMNET4af6567922->DATARMNETec0e3cb8f0);
		kfree(DATARMNET4af6567922);
	}
}
void DATARMNETcc489fbbad(struct list_head *DATARMNET3208cd0982)
{
	struct DATARMNET13c47d154e *DATARMNETf02b2a5be7;
	struct list_head *DATARMNET7b34b7b5be, *next;
	if (!DATARMNET3208cd0982) {
		DATARMNET68d84e7b98[DATARMNETac729c3d29]++;
		return;
	}
	list_for_each_safe (DATARMNET7b34b7b5be, next, DATARMNET3208cd0982) {
		DATARMNETf02b2a5be7 = list_entry(DATARMNET7b34b7b5be,
						 struct DATARMNET13c47d154e,
						 DATARMNETec0e3cb8f0);
		list_del_init(&DATARMNETf02b2a5be7->DATARMNETec0e3cb8f0);
		kfree(DATARMNETf02b2a5be7);
	}
}
void DATARMNET5157210c44(struct list_head *DATARMNETe46ae760db)
{
	struct DATARMNET47a6995138 *DATARMNETace28a2c7f;
	struct DATARMNET661e7a8566 DATARMNET3ef5021fcf;
	uint16_t idx = (0xd2d + 202 - 0xdf7);
	if (!DATARMNETe46ae760db) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x53\x43\x41\x50\x53\x3a\x20\x43\x50\x55\x20\x43\x61\x70\x61\x63\x69\x74\x69\x65\x73\x20\x4c\x69\x73\x74\x20\x69\x73\x20\x4e\x55\x4c\x4c");
		return;
	}
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x53\x43\x41\x50\x53\x3a\x20\x53\x6f\x72\x74\x65\x64\x20\x43\x50\x55\x20\x43\x61\x70\x61\x63\x69\x74\x69\x65\x73\x3a");
	list_for_each_entry (DATARMNETace28a2c7f, DATARMNETe46ae760db,
			     DATARMNET02523bfb57) {
		if (idx >= DATARMNETc6782fed88)
			break;
		rm_err("\x53\x48\x53\x5f\x53\x43\x41\x50\x53\x3a\x20\x3e\x20\x63\x70\x75\x5b\x25\x64\x5d\x20\x77\x69\x74\x68\x20\x70\x70\x73\x20\x63\x61\x70\x61\x63\x69\x74\x79\x20\x3d\x20\x25\x6c\x6c\x75\x20\x7c\x20"
		       "\x61\x76\x67\x20\x70\x70\x73\x20\x63\x61\x70\x20\x3d\x20\x25\x6c\x6c\x75\x20\x62\x70\x73\x20\x3d\x20\x25\x6c\x6c\x75",
		       DATARMNETace28a2c7f->DATARMNET42a992465f,
		       DATARMNETace28a2c7f->DATARMNET18b7a5b761,
		       DATARMNETace28a2c7f->DATARMNET4da6031170,
		       DATARMNETace28a2c7f->DATARMNET4df302dbd6);
		DATARMNET63c47f3c37[idx].DATARMNET4da6031170 =
			DATARMNETace28a2c7f->DATARMNET4da6031170;
		DATARMNET63c47f3c37[idx].DATARMNET18b7a5b761 =
			DATARMNETace28a2c7f->DATARMNET18b7a5b761;
		DATARMNET63c47f3c37[idx].DATARMNET4df302dbd6 =
			DATARMNETace28a2c7f->DATARMNET4df302dbd6;
		DATARMNET63c47f3c37[idx].DATARMNET42a992465f =
			DATARMNETace28a2c7f->DATARMNET42a992465f;
		idx += (0xd26 + 209 - 0xdf6);
	}
	rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x63\x61\x70\x5f\x64\x6d\x61\x5f\x70\x74\x72\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x61\x64\x64\x72\x20\x3d\x20\x30\x78\x25\x70\x4b"
	       "\n",
	       (unsigned long long)virt_to_phys((void *)DATARMNET410036d5ac),
	       DATARMNET410036d5ac);
	if (!DATARMNET410036d5ac) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x57\x52\x49\x54\x45\x3a\x20\x63\x61\x70\x5f\x73\x68\x61\x72\x65\x64\x20\x69\x73\x20\x4e\x55\x4c\x4c");
		return;
	}
	memcpy((char *)DATARMNET410036d5ac->data,
	       (void *)&DATARMNET63c47f3c37[(0xd2d + 202 - 0xdf7)],
	       sizeof(DATARMNET63c47f3c37));
	rm_err("\x25\x73\x28\x29\x3a\x20\x72\x6d\x65\x6d\x20\x25\x64\x20\x25\x64\x20\x25\x64\x20\x77\x6d\x65\x6d\x20\x25\x64\x20\x25\x64\x20\x25\x64"
	       "\n",
	       __func__, init_net.ipv4.sysctl_tcp_rmem[(0xd2d + 202 - 0xdf7)],
	       init_net.ipv4.sysctl_tcp_rmem[(0xd26 + 209 - 0xdf6)],
	       init_net.ipv4.sysctl_tcp_rmem[(0xd1f + 216 - 0xdf5)],
	       init_net.ipv4.sysctl_tcp_wmem[(0xd2d + 202 - 0xdf7)],
	       init_net.ipv4.sysctl_tcp_wmem[(0xd26 + 209 - 0xdf6)],
	       init_net.ipv4.sysctl_tcp_wmem[(0xd1f + 216 - 0xdf5)]);
	memset(&DATARMNET3ef5021fcf, (0xd2d + 202 - 0xdf7),
	       sizeof(DATARMNET3ef5021fcf));
	DATARMNET3ef5021fcf.DATARMNET68a58889b0[(0xd2d + 202 - 0xdf7)] =
		init_net.ipv4.sysctl_tcp_rmem[(0xd2d + 202 - 0xdf7)];
	DATARMNET3ef5021fcf.DATARMNET68a58889b0[(0xd26 + 209 - 0xdf6)] =
		init_net.ipv4.sysctl_tcp_rmem[(0xd26 + 209 - 0xdf6)];
	DATARMNET3ef5021fcf.DATARMNET68a58889b0[(0xd1f + 216 - 0xdf5)] =
		init_net.ipv4.sysctl_tcp_rmem[(0xd1f + 216 - 0xdf5)];
	DATARMNET3ef5021fcf.DATARMNET1c7c913c7b[(0xd2d + 202 - 0xdf7)] =
		init_net.ipv4.sysctl_tcp_wmem[(0xd2d + 202 - 0xdf7)];
	DATARMNET3ef5021fcf.DATARMNET1c7c913c7b[(0xd26 + 209 - 0xdf6)] =
		init_net.ipv4.sysctl_tcp_wmem[(0xd26 + 209 - 0xdf6)];
	DATARMNET3ef5021fcf.DATARMNET1c7c913c7b[(0xd1f + 216 - 0xdf5)] =
		init_net.ipv4.sysctl_tcp_wmem[(0xd1f + 216 - 0xdf5)];
	memcpy((char *)DATARMNET410036d5ac->data + sizeof(DATARMNET63c47f3c37),
	       (void *)&DATARMNET3ef5021fcf, sizeof(DATARMNET3ef5021fcf));
}
void DATARMNET0e273eab79(struct list_head *DATARMNETb436c3f30b)
{
	struct DATARMNET09a412b1c6 *DATARMNET4238158b2a;
	uint16_t idx = (0xd2d + 202 - 0xdf7);
	int DATARMNETcde08f6e7d = (0xd2d + 202 - 0xdf7);
	if (!DATARMNETb436c3f30b) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x53\x47\x4f\x4c\x44\x3a\x20\x47\x6f\x6c\x64\x20\x46\x6c\x6f\x77\x73\x20\x4c\x69\x73\x74\x20\x69\x73\x20\x4e\x55\x4c\x4c");
		return;
	}
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x53\x47\x4f\x4c\x44\x3a\x20\x4c\x69\x73\x74\x20\x6f\x66\x20\x73\x6f\x72\x74\x65\x64\x20\x67\x6f\x6c\x64\x20\x66\x6c\x6f\x77\x73\x3a");
	list_for_each_entry (DATARMNET4238158b2a, DATARMNETb436c3f30b,
			     DATARMNETd502c0412a) {
		if (DATARMNET4238158b2a->DATARMNET324c1a8f98 ==
		    (0xd2d + 202 - 0xdf7)) {
			continue;
		}
		if (idx >= DATARMNET75c69edb82) {
			break;
		}
		rm_err("\x53\x48\x53\x5f\x53\x47\x4f\x4c\x44\x3a\x20\x3e\x20\x66\x6c\x6f\x77\x20\x30\x78\x25\x78\x20\x77\x69\x74\x68\x20\x70\x70\x73\x20\x25\x6c\x6c\x75\x20\x6f\x6e\x20\x63\x70\x75\x5b\x25\x64\x5d",
		       DATARMNET4238158b2a->hash,
		       DATARMNET4238158b2a->DATARMNET324c1a8f98,
		       DATARMNET4238158b2a->DATARMNET42a992465f);
		DATARMNETcde08f6e7d += (0xd26 + 209 - 0xdf6);
		DATARMNET22b4032799[idx].DATARMNET42a992465f =
			DATARMNET4238158b2a->DATARMNET42a992465f;
		DATARMNET22b4032799[idx].hash = DATARMNET4238158b2a->hash;
		DATARMNET22b4032799[idx].DATARMNET253a9fc708 =
			DATARMNET4238158b2a->DATARMNET253a9fc708;
		DATARMNET22b4032799[idx].DATARMNET324c1a8f98 =
			DATARMNET4238158b2a->DATARMNET324c1a8f98;
		idx += (0xd26 + 209 - 0xdf6);
	}
	rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x67\x66\x6c\x6f\x77\x5f\x64\x6d\x61\x5f\x70\x74\x72\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x61\x64\x64\x72\x20\x3d\x20\x30\x78\x25\x70\x4b"
	       "\n",
	       (unsigned long long)virt_to_phys((void *)DATARMNET19c47a9f3a),
	       DATARMNET19c47a9f3a);
	if (!DATARMNET19c47a9f3a) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x57\x52\x49\x54\x45\x3a\x20\x67\x66\x6c\x6f\x77\x5f\x73\x68\x61\x72\x65\x64\x20\x69\x73\x20\x4e\x55\x4c\x4c");
		return;
	}
	rm_err("\x53\x48\x53\x5f\x53\x47\x4f\x4c\x44\x3a\x20\x6e\x75\x6d\x20\x67\x6f\x6c\x64\x20\x66\x6c\x6f\x77\x73\x20\x3d\x20\x25\x75"
	       "\n",
	       idx);
	memcpy(((char *)DATARMNET19c47a9f3a->data), &idx, sizeof(idx));
	memcpy(((char *)DATARMNET19c47a9f3a->data + sizeof(uint16_t)),
	       (void *)&DATARMNET22b4032799[(0xd2d + 202 - 0xdf7)],
	       sizeof(DATARMNET22b4032799));
}
void DATARMNETe15af8eb6d(struct list_head *DATARMNETf0fb155a9c)
{
	struct DATARMNETa52c09a590 *DATARMNET0f551e8a47;
	uint16_t idx = (0xd2d + 202 - 0xdf7);
	int DATARMNET6d96003717 = (0xd2d + 202 - 0xdf7);
	if (!DATARMNETf0fb155a9c) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x53\x4c\x4f\x57\x3a\x20\x53\x53\x20\x46\x6c\x6f\x77\x73\x20\x4c\x69\x73\x74\x20\x69\x73\x20\x4e\x55\x4c\x4c");
		return;
	}
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x53\x4c\x4f\x57\x3a\x20\x4c\x69\x73\x74\x20\x6f\x66\x20\x73\x6f\x72\x74\x65\x64\x20\x73\x73\x20\x66\x6c\x6f\x77\x73\x3a");
	list_for_each_entry (DATARMNET0f551e8a47, DATARMNETf0fb155a9c,
			     DATARMNET6f898987df) {
		if (DATARMNET0f551e8a47->DATARMNET324c1a8f98 ==
		    (0xd2d + 202 - 0xdf7)) {
			continue;
		}
		if (idx >= DATARMNET75c69edb82) {
			break;
		}
		rm_err("\x53\x48\x53\x5f\x53\x4c\x4f\x57\x3a\x20\x3e\x20\x66\x6c\x6f\x77\x20\x30\x78\x25\x78\x20\x77\x69\x74\x68\x20\x70\x70\x73\x20\x25\x6c\x6c\x75\x20\x6f\x6e\x20\x63\x70\x75\x5b\x25\x64\x5d",
		       DATARMNET0f551e8a47->hash,
		       DATARMNET0f551e8a47->DATARMNET324c1a8f98,
		       DATARMNET0f551e8a47->DATARMNET42a992465f);
		DATARMNET6d96003717 += (0xd26 + 209 - 0xdf6);
		DATARMNETb0d78d576f[idx].DATARMNET42a992465f =
			DATARMNET0f551e8a47->DATARMNET42a992465f;
		DATARMNETb0d78d576f[idx].hash = DATARMNET0f551e8a47->hash;
		DATARMNETb0d78d576f[idx].DATARMNET253a9fc708 =
			DATARMNET0f551e8a47->DATARMNET253a9fc708;
		DATARMNETb0d78d576f[idx].DATARMNET324c1a8f98 =
			DATARMNET0f551e8a47->DATARMNET324c1a8f98;
		DATARMNETb0d78d576f[idx].DATARMNETbb80fccd97 =
			DATARMNET0f551e8a47->DATARMNETbb80fccd97;
		idx += (0xd26 + 209 - 0xdf6);
	}
	rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x73\x73\x66\x6c\x6f\x77\x5f\x64\x6d\x61\x5f\x70\x74\x72\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x61\x64\x64\x72\x20\x3d\x20\x30\x78\x25\x70\x4b"
	       "\n",
	       (unsigned long long)virt_to_phys((void *)DATARMNET22e796eff3),
	       DATARMNET22e796eff3);
	if (!DATARMNET22e796eff3) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x57\x52\x49\x54\x45\x3a\x20\x73\x73\x66\x6c\x6f\x77\x5f\x73\x68\x61\x72\x65\x64\x20\x69\x73\x20\x4e\x55\x4c\x4c");
		return;
	}
	rm_err("\x53\x48\x53\x5f\x53\x4c\x4f\x57\x3a\x20\x6e\x75\x6d\x20\x73\x73\x20\x66\x6c\x6f\x77\x73\x20\x3d\x20\x25\x75"
	       "\n",
	       idx);
	memcpy(((char *)DATARMNET22e796eff3->data), &idx, sizeof(idx));
	memcpy(((char *)DATARMNET22e796eff3->data + sizeof(uint16_t)),
	       (void *)&DATARMNETb0d78d576f[(0xd2d + 202 - 0xdf7)],
	       sizeof(DATARMNETb0d78d576f));
}
void DATARMNET78f3a0ca4f(struct list_head *DATARMNET3208cd0982)
{
	struct DATARMNET13c47d154e *DATARMNETf02b2a5be7;
	uint16_t idx = (0xd2d + 202 - 0xdf7);
	int DATARMNETd31ee74d2e = (0xd2d + 202 - 0xdf7);
	if (!DATARMNET3208cd0982) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x54\x4f\x50\x3a\x20\x46\x69\x6c\x74\x65\x72\x20\x46\x6c\x6f\x77\x73\x20\x4c\x69\x73\x74\x20\x69\x73\x20\x4e\x55\x4c\x4c");
		return;
	}
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x54\x4f\x50\x3a\x20\x4c\x69\x73\x74\x20\x6f\x66\x20\x54\x4f\x50\x20\x66\x69\x6c\x74\x65\x72\x20\x66\x6c\x6f\x77\x73\x3a");
	list_for_each_entry (DATARMNETf02b2a5be7, DATARMNET3208cd0982,
			     DATARMNETec0e3cb8f0) {
		if (DATARMNETf02b2a5be7->DATARMNET324c1a8f98 ==
		    (0xd2d + 202 - 0xdf7)) {
			continue;
		}
		if (idx >= DATARMNET67578af78f) {
			break;
		}
		rm_err("\x53\x48\x53\x5f\x54\x4f\x50\x3a\x20\x3e\x20\x66\x6c\x6f\x77\x20\x30\x78\x25\x78\x20\x77\x69\x74\x68\x20\x70\x70\x73\x20\x25\x6c\x6c\x75\x20\x6f\x6e\x20\x63\x70\x75\x5b\x25\x64\x5d",
		       DATARMNETf02b2a5be7->hash,
		       DATARMNETf02b2a5be7->DATARMNET324c1a8f98,
		       DATARMNETf02b2a5be7->DATARMNET42a992465f);
		DATARMNETd31ee74d2e += (0xd26 + 209 - 0xdf6);
		memcpy(&(DATARMNET3d25ff4ef4[idx].ip_hdr),
		       &(DATARMNETf02b2a5be7->ip_hdr),
		       sizeof(DATARMNET3d25ff4ef4[idx].ip_hdr));
		memcpy(&(DATARMNET3d25ff4ef4[idx].DATARMNETe33b41dad9),
		       &(DATARMNETf02b2a5be7->DATARMNETe33b41dad9),
		       sizeof(DATARMNET3d25ff4ef4[idx].DATARMNETe33b41dad9));
		DATARMNET3d25ff4ef4[idx].DATARMNET42a992465f =
			DATARMNETf02b2a5be7->DATARMNET42a992465f;
		DATARMNET3d25ff4ef4[idx].hash = DATARMNETf02b2a5be7->hash;
		DATARMNET3d25ff4ef4[idx].DATARMNET253a9fc708 =
			DATARMNETf02b2a5be7->DATARMNET253a9fc708;
		DATARMNET3d25ff4ef4[idx].DATARMNET324c1a8f98 =
			DATARMNETf02b2a5be7->DATARMNET324c1a8f98;
		DATARMNET3d25ff4ef4[idx].DATARMNETbb80fccd97 =
			DATARMNETf02b2a5be7->DATARMNETbb80fccd97;
		DATARMNET3d25ff4ef4[idx].trans_proto =
			DATARMNETf02b2a5be7->trans_proto;
		DATARMNET3d25ff4ef4[idx].mux_id = DATARMNETf02b2a5be7->mux_id;
		DATARMNET3d25ff4ef4[idx].DATARMNET95266642d1 =
			DATARMNETf02b2a5be7->DATARMNET95266642d1;
		DATARMNET3d25ff4ef4[idx].DATARMNET0d682bcb29 =
			DATARMNETf02b2a5be7->DATARMNET0d682bcb29;
		DATARMNET3d25ff4ef4[idx].DATARMNET20e8fc9db8 =
			DATARMNETf02b2a5be7->DATARMNET20e8fc9db8;
		idx += (0xd26 + 209 - 0xdf6);
	}
	rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x66\x66\x6c\x6f\x77\x5f\x64\x6d\x61\x5f\x70\x74\x72\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x61\x64\x64\x72\x20\x3d\x20\x30\x78\x25\x70\x4b"
	       "\n",
	       (unsigned long long)virt_to_phys((void *)DATARMNET9b8000d2a7),
	       DATARMNET9b8000d2a7);
	if (!DATARMNET9b8000d2a7) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x57\x52\x49\x54\x45\x3a\x20\x66\x66\x6c\x6f\x77\x5f\x73\x68\x61\x72\x65\x64\x20\x69\x73\x20\x4e\x55\x4c\x4c");
		return;
	}
	rm_err("\x53\x48\x53\x5f\x53\x4c\x4f\x57\x3a\x20\x6e\x75\x6d\x20\x66\x66\x6c\x6f\x77\x73\x20\x3d\x20\x25\x75"
	       "\n",
	       idx);
	memcpy(((char *)DATARMNET9b8000d2a7->data), &idx, sizeof(idx));
	memcpy(((char *)DATARMNET9b8000d2a7->data + sizeof(uint16_t)),
	       (void *)&DATARMNET3d25ff4ef4[(0xd2d + 202 - 0xdf7)],
	       sizeof(DATARMNET3d25ff4ef4));
}
void DATARMNETa3d6c4072d(struct list_head *DATARMNET922b4752e2)
{
	struct DATARMNETe176be7f7c *DATARMNET4af6567922;
	uint16_t idx = (0xd2d + 202 - 0xdf7);
	int DATARMNET5c1d9cb9dd = (0xd2d + 202 - 0xdf7);
	if (!DATARMNET922b4752e2) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x54\x4f\x50\x3a\x20\x4c\x4c\x20\x46\x6c\x6f\x77\x73\x20\x4c\x69\x73\x74\x20\x69\x73\x20\x4e\x55\x4c\x4c");
		return;
	}
	rm_err("\x25\x73",
	       "\x53\x48\x53\x5f\x54\x4f\x50\x3a\x20\x4c\x69\x73\x74\x20\x6f\x66\x20\x54\x4f\x50\x20\x4c\x4c\x20\x66\x69\x6c\x74\x65\x72\x20\x66\x6c\x6f\x77\x73\x3a");
	list_for_each_entry (DATARMNET4af6567922, DATARMNET922b4752e2,
			     DATARMNET7287946e7b) {
		if (idx >= DATARMNETa847b6b6d0) {
			break;
		}
		rm_err("\x53\x48\x53\x5f\x54\x4f\x50\x3a\x20\x3e\x20\x66\x6c\x6f\x77\x20\x30\x78\x25\x78\x20\x77\x69\x74\x68\x20\x70\x70\x73\x20\x25\x6c\x6c\x75\x20\x6f\x6e\x20\x63\x70\x75\x5b\x25\x64\x5d",
		       DATARMNET4af6567922->hash,
		       DATARMNET4af6567922->DATARMNET324c1a8f98,
		       DATARMNET4af6567922->DATARMNET42a992465f);
		DATARMNET5c1d9cb9dd += (0xd26 + 209 - 0xdf6);
		memcpy(&(DATARMNET2fdef9da24[idx].ip_hdr),
		       &(DATARMNET4af6567922->ip_hdr),
		       sizeof(DATARMNET2fdef9da24[idx].ip_hdr));
		memcpy(&(DATARMNET2fdef9da24[idx].DATARMNETe33b41dad9),
		       &(DATARMNET4af6567922->DATARMNETe33b41dad9),
		       sizeof(DATARMNET2fdef9da24[idx].DATARMNETe33b41dad9));
		DATARMNET2fdef9da24[idx].DATARMNET42a992465f =
			DATARMNET4af6567922->DATARMNET42a992465f;
		DATARMNET2fdef9da24[idx].hash = DATARMNET4af6567922->hash;
		DATARMNET2fdef9da24[idx].DATARMNET253a9fc708 =
			DATARMNET4af6567922->DATARMNET253a9fc708;
		DATARMNET2fdef9da24[idx].DATARMNET324c1a8f98 =
			DATARMNET4af6567922->DATARMNET324c1a8f98;
		DATARMNET2fdef9da24[idx].DATARMNETda8e8ae7ed =
			DATARMNET4af6567922->DATARMNETda8e8ae7ed;
		DATARMNET2fdef9da24[idx].DATARMNETbb80fccd97 =
			DATARMNET4af6567922->DATARMNETbb80fccd97;
		DATARMNET2fdef9da24[idx].trans_proto =
			DATARMNET4af6567922->trans_proto;
		DATARMNET2fdef9da24[idx].mux_id = DATARMNET4af6567922->mux_id;
		DATARMNET2fdef9da24[idx].DATARMNET95266642d1 =
			DATARMNET4af6567922->DATARMNET95266642d1;
		DATARMNET2fdef9da24[idx].DATARMNET0d682bcb29 =
			DATARMNET4af6567922->DATARMNET0d682bcb29;
		DATARMNET2fdef9da24[idx].DATARMNET20e8fc9db8 =
			DATARMNET4af6567922->DATARMNET20e8fc9db8;
		idx += (0xd26 + 209 - 0xdf6);
	}
	rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x6c\x6c\x5f\x66\x6c\x6f\x77\x5f\x64\x6d\x61\x5f\x70\x74\x72\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x61\x64\x64\x72\x20\x3d\x20\x30\x78\x25\x70\x4b"
	       "\n",
	       (unsigned long long)virt_to_phys((void *)DATARMNET835a28686c),
	       DATARMNET835a28686c);
	if (!DATARMNET835a28686c) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x57\x52\x49\x54\x45\x3a\x20\x6c\x6c\x66\x6c\x6f\x77\x5f\x73\x68\x61\x72\x65\x64\x20\x69\x73\x20\x4e\x55\x4c\x4c");
		return;
	}
	rm_err("\x53\x48\x53\x5f\x53\x4c\x4f\x57\x3a\x20\x6e\x75\x6d\x20\x6c\x6c\x66\x6c\x6f\x77\x73\x20\x3d\x20\x25\x75"
	       "\n",
	       idx);
	memcpy(((char *)DATARMNET835a28686c->data), &idx, sizeof(idx));
	memcpy(((char *)DATARMNET835a28686c->data + sizeof(uint16_t)),
	       (void *)&DATARMNET2fdef9da24[(0xd2d + 202 - 0xdf7)],
	       sizeof(DATARMNET2fdef9da24));
}
void DATARMNET78666f33a1(void)
{
	struct rmnet_priv *priv;
	struct DATARMNET9b44b71ee9 *ep = NULL;
	u16 idx = (0xd2d + 202 - 0xdf7);
	u16 count = (0xd2d + 202 - 0xdf7);
	rm_err("\x53\x48\x53\x5f\x4e\x45\x54\x44\x45\x56\x3a\x20\x66\x75\x6e\x63\x74\x69\x6f\x6e\x20\x65\x6e\x74\x65\x72\x20\x25\x75"
	       "\n",
	       idx);
	list_for_each_entry (ep, &DATARMNET30a3e83974, DATARMNET0763436b8d) {
		count += (0xd26 + 209 - 0xdf6);
		rm_err("\x53\x48\x53\x5f\x4e\x45\x54\x44\x45\x56\x3a\x20\x66\x75\x6e\x63\x74\x69\x6f\x6e\x20\x65\x6e\x74\x65\x72\x20\x65\x70\x20\x25\x75"
		       "\n",
		       count);
		if (!ep->DATARMNET4a4e6f66b5) {
			rm_err("\x53\x48\x53\x5f\x4e\x45\x54\x44\x45\x56\x3a\x20\x65\x70\x20\x25\x75\x20\x69\x73\x20\x4e\x4f\x54\x20\x61\x63\x74\x69\x76\x65"
			       "\n",
			       count);
			continue;
		}
		rm_err("\x53\x48\x53\x5f\x4e\x45\x54\x44\x45\x56\x3a\x20\x65\x70\x20\x25\x75\x20\x69\x73\x20\x61\x63\x74\x69\x76\x65\x20\x61\x6e\x64\x20\x6e\x6f\x74\x20\x6e\x75\x6c\x6c"
		       "\n",
		       count);
		if (idx >= DATARMNETe4d15b9332) {
			break;
		}
		priv = netdev_priv(ep->ep);
		if (!priv) {
			rm_err("\x53\x48\x53\x5f\x4e\x45\x54\x44\x45\x56\x3a\x20\x70\x72\x69\x76\x20\x66\x6f\x72\x20\x65\x70\x20\x25\x75\x20\x69\x73\x20\x6e\x75\x6c\x6c"
			       "\n",
			       count);
			continue;
		}
		rm_err("\x53\x48\x53\x5f\x4e\x45\x54\x44\x45\x56\x3a\x20\x65\x70\x20\x25\x75\x20\x68\x61\x73\x20\x6e\x61\x6d\x65\x20\x3d\x20\x25\x73\x20"
		       "\n",
		       count, ep->ep->name);
		rm_err("\x53\x48\x53\x5f\x4e\x45\x54\x44\x45\x56\x3a\x20\x65\x70\x20\x25\x75\x20\x68\x61\x73\x20\x6d\x75\x78\x5f\x69\x64\x20\x3d\x20\x25\x75\x20"
		       "\n",
		       count, priv->mux_id);
		rm_err("\x53\x48\x53\x5f\x4e\x45\x54\x44\x45\x56\x3a\x20\x65\x70\x20\x25\x75\x20\x68\x61\x73\x20\x69\x70\x5f\x6d\x69\x73\x73\x20\x3d\x20\x25\x6c\x75\x20"
		       "\n",
		       count, priv->stats.coal.close.ip_miss);
		rm_err("\x53\x48\x53\x5f\x4e\x45\x54\x44\x45\x56\x3a\x20\x65\x70\x20\x25\x75\x20\x68\x61\x73\x20\x63\x6f\x61\x6c\x5f\x72\x78\x5f\x70\x6b\x74\x73\x20\x3d\x20\x25\x6c\x75\x20"
		       "\n",
		       count, priv->stats.coal.coal_pkts);
		rm_err("\x53\x48\x53\x5f\x4e\x45\x54\x44\x45\x56\x3a\x20\x65\x70\x20\x25\x75\x20\x68\x61\x73\x20\x75\x64\x70\x5f\x72\x78\x5f\x62\x70\x73\x20\x3d\x20\x25\x6c\x75\x20"
		       "\n",
		       count, ep->DATARMNET257fc4b2d4);
		rm_err("\x53\x48\x53\x5f\x4e\x45\x54\x44\x45\x56\x3a\x20\x65\x70\x20\x25\x75\x20\x68\x61\x73\x20\x74\x63\x70\x5f\x72\x78\x5f\x62\x70\x73\x20\x3d\x20\x25\x6c\x75\x20"
		       "\n",
		       count, ep->DATARMNET4eb77c78e6);
		DATARMNETf46265286b[idx].DATARMNET77de6e34f2 =
			priv->stats.coal.close.ip_miss;
		DATARMNETf46265286b[idx].hw_evict =
			priv->stats.coal.close.hw_evict;
		DATARMNETf46265286b[idx].coal_tcp = priv->stats.coal.coal_tcp;
		DATARMNETf46265286b[idx].coal_tcp_bytes =
			priv->stats.coal.coal_tcp_bytes;
		DATARMNETf46265286b[idx].coal_udp = priv->stats.coal.coal_udp;
		DATARMNETf46265286b[idx].coal_udp_bytes =
			priv->stats.coal.coal_udp_bytes;
		DATARMNETf46265286b[idx].mux_id = priv->mux_id;
		strlcpy(DATARMNETf46265286b[idx].name, ep->ep->name,
			sizeof(DATARMNETf46265286b[idx].name));
		DATARMNETf46265286b[idx].DATARMNET870c3dafcb =
			priv->stats.coal.coal_pkts;
		DATARMNETf46265286b[idx].DATARMNET4eb77c78e6 =
			ep->DATARMNET4eb77c78e6;
		DATARMNETf46265286b[idx].DATARMNET257fc4b2d4 =
			ep->DATARMNET257fc4b2d4;
		idx += (0xd26 + 209 - 0xdf6);
	}
	rm_err("\x53\x48\x53\x5f\x4d\x45\x4d\x3a\x20\x6e\x65\x74\x64\x65\x76\x5f\x73\x68\x61\x72\x65\x64\x20\x3d\x20\x30\x78\x25\x6c\x6c\x78\x20\x61\x64\x64\x72\x20\x3d\x20\x30\x78\x25\x70\x4b"
	       "\n",
	       (unsigned long long)virt_to_phys((void *)DATARMNET67d31dc40a),
	       DATARMNET67d31dc40a);
	if (!DATARMNET67d31dc40a) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x57\x52\x49\x54\x45\x3a\x20\x6e\x65\x74\x64\x65\x76\x5f\x73\x68\x61\x72\x65\x64\x20\x69\x73\x20\x4e\x55\x4c\x4c");
		return;
	}
	memcpy(((char *)DATARMNET67d31dc40a->data), &idx, sizeof(idx));
	memcpy(((char *)DATARMNET67d31dc40a->data + sizeof(uint16_t)),
	       (void *)&DATARMNETf46265286b[(0xd2d + 202 - 0xdf7)],
	       sizeof(DATARMNETf46265286b));
}
void DATARMNETf5f83b943f(void)
{
	kuid_t DATARMNETdaca088404;
	kgid_t DATARMNET254aa091f2;
	DATARMNETe4c5563cdb = proc_mkdir("\x73\x68\x73", NULL);
	if (!DATARMNETe4c5563cdb) {
		rm_err("\x25\x73",
		       "\x53\x48\x53\x5f\x4d\x45\x4d\x5f\x49\x4e\x49\x54\x3a\x20\x46\x61\x69\x6c\x65\x64\x20\x74\x6f\x20\x63\x72\x65\x61\x74\x65\x20\x70\x72\x6f\x63\x20\x64\x69\x72");
		return;
	}
	DATARMNETdaca088404 = make_kuid(&init_user_ns, (0xdc7 + 2646 - 0x1434));
	DATARMNET254aa091f2 = make_kgid(&init_user_ns, (0xdc7 + 2646 - 0x1434));
	if (uid_valid(DATARMNETdaca088404) && gid_valid(DATARMNET254aa091f2))
		proc_set_user(DATARMNETe4c5563cdb, DATARMNETdaca088404,
			      DATARMNET254aa091f2);

	/* Initialize buffers before users can open the proc entries. */
	DATARMNET6bf538fa23();
	DATARMNET410036d5ac = NULL;
	DATARMNET19c47a9f3a = NULL;
	DATARMNET22e796eff3 = NULL;
	DATARMNET9b8000d2a7 = NULL;
	DATARMNET835a28686c = NULL;
	DATARMNET67d31dc40a = NULL;
	DATARMNETaea4c85748();

	proc_create(DATARMNET41be983a65, (0xdb7 + 6665 - 0x261c),
		    DATARMNETe4c5563cdb, &DATARMNET668485bd14);
	proc_create(DATARMNET5ddc91451c, (0xdb7 + 6665 - 0x261c),
		    DATARMNETe4c5563cdb, &DATARMNETe84fbaa6af);
	proc_create(DATARMNETeb2a21dd7c, (0xdb7 + 6665 - 0x261c),
		    DATARMNETe4c5563cdb, &DATARMNET8fe5f892a8);
	proc_create(DATARMNET1c4ea23858, (0xdb7 + 6665 - 0x261c),
		    DATARMNETe4c5563cdb, &DATARMNET0104d40d4b);
	proc_create(DATARMNET8b29e14112, (0xdb7 + 6665 - 0x261c),
		    DATARMNETe4c5563cdb, &DATARMNETddcdf7bd4e);
	proc_create(DATARMNETe98d39b779, (0xdb7 + 6665 - 0x261c),
		    DATARMNETe4c5563cdb, &DATARMNET6eb63d9ad0);
}
void DATARMNET28d33bd09f(void)
{
	remove_proc_entry(DATARMNET41be983a65, DATARMNETe4c5563cdb);
	remove_proc_entry(DATARMNET5ddc91451c, DATARMNETe4c5563cdb);
	remove_proc_entry(DATARMNETeb2a21dd7c, DATARMNETe4c5563cdb);
	remove_proc_entry(DATARMNET1c4ea23858, DATARMNETe4c5563cdb);
	remove_proc_entry(DATARMNET8b29e14112, DATARMNETe4c5563cdb);
	remove_proc_entry(DATARMNETe98d39b779, DATARMNETe4c5563cdb);
	remove_proc_entry(DATARMNET6517f07a36, NULL);
	DATARMNET6bf538fa23();
	DATARMNET410036d5ac = NULL;
	DATARMNET19c47a9f3a = NULL;
	DATARMNET22e796eff3 = NULL;
	DATARMNET9b8000d2a7 = NULL;
	DATARMNET835a28686c = NULL;
	DATARMNET67d31dc40a = NULL;
	DATARMNETaea4c85748();
}

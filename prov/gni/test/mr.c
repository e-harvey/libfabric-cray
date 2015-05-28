/*
 * Copyright (c) 2015 Cray Inc. All rights reserved.
 *
 *  Created on: May 11, 2015
 *      Author: jswaro
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <time.h>
#include <string.h>

#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_cm.h>

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "gnix_cq.h"
#include "gnix.h"

#include <criterion/criterion.h>

static struct fid_fabric *fab;
static struct fid_domain *dom;
static struct fid_ep *ep;
static struct fid_mr *mr;
static struct fi_info *hints;
static struct fi_info *fi;
static struct fi_cq_attr cq_attr;

#define __BUF_LEN 4096
static unsigned char *buf;
static int buf_len = __BUF_LEN * sizeof(unsigned char);

uint64_t default_access = (FI_REMOTE_READ | FI_REMOTE_WRITE
		| FI_READ | FI_WRITE);

uint64_t default_flags = 0;
uint64_t default_req_key = 0;
uint64_t default_offset = 0;

static void mr_setup(void)
{
	int ret = 0;

	hints = fi_allocinfo();
	cr_assert(hints, "fi_allocinfo");

	hints->domain_attr->cq_data_size = 4;
	hints->mode = ~0;

	hints->fabric_attr->name = strdup("gni");

	ret = fi_getinfo(FI_VERSION(1, 0), NULL, 0, 0, hints, &fi);
	cr_assert(!ret, "fi_getinfo");

	ret = fi_fabric(fi->fabric_attr, &fab, NULL);
	cr_assert(!ret, "fi_fabric");

	ret = fi_domain(fab, fi, &dom, NULL);
	cr_assert(!ret, "fi_domain");

	ret = fi_endpoint(dom, fi, &ep, NULL);
	cr_assert(!ret, "fi_endpoint");

	cq_attr.wait_obj = FI_WAIT_NONE;

	buf = calloc(__BUF_LEN, sizeof(unsigned char));
	cr_assert(buf, "buffer allocation");
}

static void mr_teardown(void)
{
	int ret = 0;

	ret = fi_close(&ep->fid);
	cr_assert(!ret, "failure in closing ep.");
	ret = fi_close(&dom->fid);
	cr_assert(!ret, "failure in closing domain.");
	ret = fi_close(&fab->fid);
	cr_assert(!ret, "failure in closing fabric.");
	fi_freeinfo(fi);
	fi_freeinfo(hints);

	free(buf);
}

TestSuite(memory_registration_bare, .init = mr_setup, .fini = mr_teardown);

Test(memory_registration_bare, basic_init)
{
	int ret;

	ret = fi_mr_reg(dom, (void *) buf, buf_len, default_access,
			default_offset, default_req_key,
			default_flags, &mr, NULL);
	cr_assert(ret == FI_SUCCESS);

	ret = fi_close(&mr->fid);
	cr_assert(ret == FI_SUCCESS);
}

Test(memory_registration_bare, invalid_flags)
{
	int ret;

	ret = fi_mr_reg(dom, (void *) buf, buf_len, default_access,
			default_offset, default_req_key,
			~0, &mr, NULL);
	cr_assert(ret == -FI_EBADFLAGS);
}

Test(memory_registration_bare, invalid_access)
{
	int ret;

	ret = fi_mr_reg(dom, (void *) buf, buf_len, 0,
			default_offset, default_req_key,
			default_flags, &mr, NULL);
	cr_assert(ret == -FI_EINVAL);
}

Test(memory_registration_bare, invalid_offset)
{
	int ret;

	ret = fi_mr_reg(dom, (void *) buf, buf_len, default_access,
			~0, default_req_key, default_flags,
			&mr, NULL);
	cr_assert(ret == -FI_EINVAL);
}

Test(memory_registration_bare, invalid_requested_key)
{
	int ret;

	ret = fi_mr_reg(dom, (void *) buf, buf_len, default_access,
			default_offset, ~0, default_flags,
			&mr, NULL);
	cr_assert(ret == -FI_EKEYREJECTED);
}

Test(memory_registration_bare, invalid_buf)
{
	int ret;

	ret = fi_mr_reg(dom, NULL, buf_len, default_access,
			default_offset, default_req_key, default_flags,
			&mr, NULL);
	cr_assert(ret == -FI_EINVAL);
}

Test(memory_registration_bare, invalid_mr_ptr)
{
	int ret;

	ret = fi_mr_reg(dom, (void *) buf, buf_len, default_access,
			default_offset, default_req_key, default_flags,
			NULL, NULL);
	cr_assert(ret == -FI_EINVAL);
}

Test(memory_registration_bare, invalid_fid_class)
{
	int ret;
	size_t old_class = dom->fid.fclass;

	dom->fid.fclass = FI_CLASS_UNSPEC;

	ret = fi_mr_reg(dom, (void *) buf, buf_len, default_access,
			default_offset, default_req_key, default_flags,
			&mr, NULL);
	cr_assert(ret == -FI_EINVAL);

	/* restore old fclass for teardown */
	dom->fid.fclass = old_class;
}


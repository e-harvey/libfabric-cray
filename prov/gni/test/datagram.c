/*
 * Copyright (c) 2015 Los Alamos National Security, LLC. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_cm.h>

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "gnix_datagram.h"
#include "gnix_cm_nic.h"

#ifdef assert
#undef assert
#endif

#include <criterion/criterion.h>

static struct fid_fabric *fab;
static struct fid_domain *dom;
static struct fid_ep *ep;
static struct fi_info *hints;
static struct fi_info *fi;
static struct gnix_fid_ep *ep_priv;

void dg_setup(void)
{
	int ret = 0;

	hints = fi_allocinfo();
	assert(hints, "fi_allocinfo");

	hints->domain_attr->cq_data_size = 4;
	hints->mode = ~0;

	hints->fabric_attr->name = strdup("gni");

	ret = fi_getinfo(FI_VERSION(1, 0), NULL, 0, 0, hints, &fi);
	assert(!ret, "fi_getinfo");

	ret = fi_fabric(fi->fabric_attr, &fab, NULL);
	assert(!ret, "fi_fabric");

	ret = fi_domain(fab, fi, &dom, NULL);
	assert(!ret, "fi_domain");

	ret = fi_endpoint(dom, fi, &ep, NULL);
	assert(!ret, "fi_endpoint");
}

void dg_setup_prog_manual(void)
{
	int ret = 0;

	hints = fi_allocinfo();
	assert(hints, "fi_allocinfo");

	hints->domain_attr->cq_data_size = 4;
	hints->domain_attr->control_progress = FI_PROGRESS_MANUAL;
	hints->mode = ~0;

	hints->fabric_attr->name = strdup("gni");

	ret = fi_getinfo(FI_VERSION(1, 0), NULL, 0, 0, hints, &fi);
	assert(!ret, "fi_getinfo");

       ret = fi_fabric(fi->fabric_attr, &fab, NULL);
	assert(!ret, "fi_fabric");

	ret = fi_domain(fab, fi, &dom, NULL);
	assert(!ret, "fi_domain");

	ret = fi_endpoint(dom, fi, &ep, NULL);
	assert(!ret, "fi_endpoint");
}

void dg_teardown(void)
{
	int ret = 0;

	ret = fi_close(&ep->fid);
	assert(!ret, "failure in closing ep.");
	ret = fi_close(&dom->fid);
	assert(!ret, "failure in closing domain.");
	ret = fi_close(&fab->fid);
	assert(!ret, "failure in closing fabric.");
	fi_freeinfo(fi);
	fi_freeinfo(hints);
}

/*******************************************************************************
 * Allocation Tests:
 *
 * try different datagram allocation/free patterns and see if something
 * explodes.
 ******************************************************************************/

TestSuite(dg_allocation, .init = dg_setup, .fini = dg_teardown);

Test(dg_allocation, dgram_alloc_wc)
{
	int ret = 0, i;
	struct gnix_cm_nic *cm_nic;
	struct gnix_datagram **dgram_ptr;
	struct gnix_fid_fabric *fab_priv;

	ep_priv = container_of(ep, struct gnix_fid_ep, ep_fid);
	cm_nic = ep_priv->cm_nic;
	assert((cm_nic != NULL), "cm_nic NULL");

	assert((cm_nic->dgram_hndl != NULL), "cm_nic dgram_hndl NULL");

	fab_priv = container_of(fab, struct gnix_fid_fabric, fab_fid);

	dgram_ptr = calloc(fab_priv->n_wc_dgrams,
			   sizeof(struct gnix_datagram *));
	assert((dgram_ptr != NULL), "calloc failed");

	for (i = 0; i < fab_priv->n_wc_dgrams; i++) {
		ret = _gnix_dgram_alloc(cm_nic->dgram_hndl, GNIX_DGRAM_WC,
					&dgram_ptr[i]);
		assert(!ret, "_gnix_dgram_alloc wc");
	}

	for (i = 0; i < fab_priv->n_wc_dgrams; i++) {
		ret = _gnix_dgram_free(dgram_ptr[i]);
		assert(!ret, "_gnix_dgram_free wc");
	}

	free(dgram_ptr);
}

Test(dg_allocation, dgram_alloc_wc_alt)
{
	int ret = 0, i;
	struct gnix_cm_nic *cm_nic;
	struct gnix_datagram *dgram_ptr;
	struct gnix_fid_fabric *fab_priv;

	ep_priv = container_of(ep, struct gnix_fid_ep, ep_fid);
	cm_nic = ep_priv->cm_nic;
	assert((cm_nic != NULL), "cm_nic NULL");

	assert((cm_nic->dgram_hndl != NULL), "cm_nic dgram_hndl NULL");

	fab_priv = container_of(fab, struct gnix_fid_fabric, fab_fid);

	for (i = 0; i < fab_priv->n_wc_dgrams; i++) {
		ret = _gnix_dgram_alloc(cm_nic->dgram_hndl, GNIX_DGRAM_WC,
					&dgram_ptr);
		assert(!ret, "_gnix_dgram_alloc wc");
		ret = _gnix_dgram_free(dgram_ptr);
		assert(!ret, "_gnix_dgram_free wc");
	}
}

Test(dg_allocation, dgram_alloc_bnd)
{
	int ret = 0, i;
	struct gnix_cm_nic *cm_nic;
	struct gnix_datagram **dgram_ptr;
	struct gnix_fid_fabric *fab_priv;

	ep_priv = container_of(ep, struct gnix_fid_ep, ep_fid);
	cm_nic = ep_priv->cm_nic;
	assert((cm_nic != NULL), "cm_nic NULL");

	assert((cm_nic->dgram_hndl != NULL), "cm_nic dgram_hndl NULL");

	fab_priv = container_of(fab, struct gnix_fid_fabric, fab_fid);

	dgram_ptr = calloc(fab_priv->n_bnd_dgrams,
			   sizeof(struct gnix_datagram *));
	assert((dgram_ptr != NULL), "calloc failed");

	for (i = 0; i < fab_priv->n_bnd_dgrams; i++) {
		ret = _gnix_dgram_alloc(cm_nic->dgram_hndl, GNIX_DGRAM_BND,
					&dgram_ptr[i]);
		assert(!ret, "_gnix_dgram_alloc bnd");
	}

	for (i = 0; i < fab_priv->n_wc_dgrams; i++) {
		ret = _gnix_dgram_free(dgram_ptr[i]);
		assert(!ret, "_gnix_dgram_free bnd");
	}

	free(dgram_ptr);
}

Test(dg_allocation, dgram_alloc_wc_bnd)
{
	int ret = 0, i;
	struct gnix_cm_nic *cm_nic;
	struct gnix_datagram *dgram_ptr;
	struct gnix_fid_fabric *fab_priv;

	ep_priv = container_of(ep, struct gnix_fid_ep, ep_fid);
	cm_nic = ep_priv->cm_nic;
	assert((cm_nic != NULL), "cm_nic NULL");

	assert((cm_nic->dgram_hndl != NULL), "cm_nic dgram_hndl NULL");

	fab_priv = container_of(fab, struct gnix_fid_fabric, fab_fid);

	for (i = 0; i < fab_priv->n_bnd_dgrams; i++) {
		ret = _gnix_dgram_alloc(cm_nic->dgram_hndl, GNIX_DGRAM_BND,
					&dgram_ptr);
		assert(!ret, "_gnix_dgram_alloc bnd");
		ret = _gnix_dgram_free(dgram_ptr);
		assert(!ret, "_gnix_dgram_free bnd");
	}
}

Test(dg_allocation, dgram_pack_unpack)
{
	int ret = 0;
	ssize_t len;
	struct gnix_cm_nic *cm_nic;
	struct gnix_datagram *dgram_ptr;
	char in_buf[] = "0xdeadbeef";
	char out_buf[GNI_DATAGRAM_MAXSIZE];

	ep_priv = container_of(ep, struct gnix_fid_ep, ep_fid);
	cm_nic = ep_priv->cm_nic;
	assert((cm_nic != NULL), "cm_nic NULL");

	assert((cm_nic->dgram_hndl != NULL), "cm_nic dgram_hndl NULL");

	ret = _gnix_dgram_alloc(cm_nic->dgram_hndl, GNIX_DGRAM_BND,
				&dgram_ptr);
	assert(!ret, "_gnix_dgram_alloc bnd");

	/*
	 * check pack/unpack for GNIX_DGRAM_IN_BUF
	 */

	len = _gnix_dgram_pack_buf(dgram_ptr, GNIX_DGRAM_IN_BUF,
					in_buf, sizeof(in_buf));
	assert(len > 0);
	assert_eq(len, (ssize_t)sizeof(in_buf));

	len = _gnix_dgram_unpack_buf(dgram_ptr, GNIX_DGRAM_IN_BUF,
					out_buf, sizeof(in_buf));
	assert(len > 0);
	assert_eq(len, (ssize_t)sizeof(in_buf));

	assert_eq(0, strcmp(in_buf, out_buf));

	/*
	 * check pack/unpack for GNIX_DGRAM_OUT_BUF
	 */

	len = _gnix_dgram_pack_buf(dgram_ptr, GNIX_DGRAM_OUT_BUF,
					in_buf, sizeof(in_buf));
	assert(len > 0);
	assert_eq(len, (ssize_t)sizeof(in_buf));

	memset(out_buf, 0, sizeof(out_buf));

	len = _gnix_dgram_unpack_buf(dgram_ptr, GNIX_DGRAM_OUT_BUF,
					out_buf, sizeof(in_buf));
	assert(len > 0);
	assert_eq(len, (ssize_t)sizeof(in_buf));

	assert_eq(0, strcmp(in_buf, out_buf));

	ret = _gnix_dgram_free(dgram_ptr);
	assert(!ret, "_gnix_dgram_free bnd");

}

static struct gnix_address local_address;
static int dgram_match;

static int dgram_callback_fn(struct gnix_datagram *the_dgram,
			     struct gnix_address where_from,
			     gni_post_state_t dgram_state)
{
	if (dgram_state != GNI_POST_COMPLETED) {
		fprintf(stderr, "dgram_state check failed %s %d\n",
			__func__, __LINE__);
		return -FI_EIO;
	}

	if ((where_from.device_addr != local_address.device_addr) ||
	    (where_from.cdm_id != local_address.cdm_id)) {
		fprintf(stderr, "where from check failed %s %d\n",
			__func__, __LINE__);
		return -FI_EIO;
	}

	dgram_match = 1;
	return 0;
}

Test(dg_allocation,  dgram_wc_post_exchg)
{
	int ret = 0;
	gni_return_t status;
	struct gnix_cm_nic *cm_nic;
	struct gnix_datagram *dgram_wc, *dgram_bnd;

	ep_priv = container_of(ep, struct gnix_fid_ep, ep_fid);
	cm_nic = ep_priv->cm_nic;
	assert((cm_nic != NULL), "cm_nic NULL");

	assert((cm_nic->dgram_hndl != NULL), "cm_nic dgram_hndl NULL");

	ret = _gnix_dgram_alloc(cm_nic->dgram_hndl, GNIX_DGRAM_WC,
				&dgram_wc);
	assert(!ret, "_gnix_dgram_alloc wc");

	dgram_wc->callback_fn = dgram_callback_fn;
	ret = _gnix_dgram_wc_post(dgram_wc, &status);
	assert((ret == 0), "_gnix_dgram_alloc wc");

	ret = _gnix_dgram_alloc(cm_nic->dgram_hndl, GNIX_DGRAM_BND,
				&dgram_bnd);
	assert((ret == 0), "_gnix_dgram_alloc bnd");

	dgram_bnd->target_addr.device_addr = cm_nic->device_addr;
	dgram_bnd->target_addr.cdm_id = cm_nic->cdm_id;

	local_address.device_addr = cm_nic->device_addr;
	local_address.cdm_id = cm_nic->cdm_id;

	dgram_bnd->callback_fn = dgram_callback_fn;
	ret = _gnix_dgram_bnd_post(dgram_bnd, &status);
	assert(ret == 0);

	/*
	 * progress auto, don't need to do anything
	 */
	while (dgram_match != 1) {
		ret = _gnix_cm_nic_progress(cm_nic);
		assert(ret == 0);
		pthread_yield();
	}

	ret = _gnix_dgram_free(dgram_bnd);
	assert(!ret, "_gnix_dgram_free bnd");

	ret = _gnix_dgram_free(dgram_wc);
	assert(!ret, "_gnix_dgram_free wc");

}

Test(dg_allocation,  dgram_wc_post_exchg_manual, .init = dg_setup_prog_manual)
{
	int ret = 0;
	gni_return_t status;
	struct gnix_cm_nic *cm_nic;
	struct gnix_datagram *dgram_wc, *dgram_bnd;

	ep_priv = container_of(ep, struct gnix_fid_ep, ep_fid);
	cm_nic = ep_priv->cm_nic;
	assert((cm_nic != NULL), "cm_nic NULL");

	assert(cm_nic->control_progress == FI_PROGRESS_MANUAL);

	assert((cm_nic->dgram_hndl != NULL), "cm_nic dgram_hndl NULL");

	ret = _gnix_dgram_alloc(cm_nic->dgram_hndl, GNIX_DGRAM_WC,
				&dgram_wc);
	assert(!ret, "_gnix_dgram_alloc wc");

	dgram_wc->callback_fn = dgram_callback_fn;
	ret = _gnix_dgram_wc_post(dgram_wc, &status);
	assert((ret == 0), "_gnix_dgram_alloc wc");

	ret = _gnix_dgram_alloc(cm_nic->dgram_hndl, GNIX_DGRAM_BND,
				&dgram_bnd);
	assert((ret == 0), "_gnix_dgram_alloc bnd");

	dgram_bnd->target_addr.device_addr = cm_nic->device_addr;
	dgram_bnd->target_addr.cdm_id = cm_nic->cdm_id;

	local_address.device_addr = cm_nic->device_addr;
	local_address.cdm_id = cm_nic->cdm_id;

	dgram_bnd->callback_fn = dgram_callback_fn;
	ret = _gnix_dgram_bnd_post(dgram_bnd, &status);
	assert(ret == 0);

	/*
	 * progress auto, don't need to do anything
	 */
	while (dgram_match != 1) {
		ret = _gnix_cm_nic_progress(cm_nic);
		assert(ret == 0);
		pthread_yield();
	}

	ret = _gnix_dgram_free(dgram_bnd);
	assert(!ret, "_gnix_dgram_free bnd");

	ret = _gnix_dgram_free(dgram_wc);
	assert(!ret, "_gnix_dgram_free wc");

}

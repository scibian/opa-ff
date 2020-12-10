/* BEGIN_ICS_COPYRIGHT2 ****************************************

Copyright (c) 2015-2020, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

** END_ICS_COPYRIGHT2   ****************************************/

/* [ICS VERSION STRING: unknown] */

/*
 * This program is meant to determine the performance of the path query system. 
 * The destination file is generated by the perf script: 
 *     ./build_table.pl
 * The resulting file guidtable_xxxxx can be used with the tool: 
 *     ./opa_osd_perf -q 20000000 -p 0x8001 guidtable_xxxxx 
 */
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <infiniband/umad.h>
#include "opasadb_path.h"
#include "opasadb_debug.h"

#define MAX_SOURCE_PORTS (UMAD_CA_MAX_PORTS * UMAD_MAX_DEVICES)
#define MAX_SIDS 8
#define MAX_PKEYS 8


#define DELIMITER ";"

static 	char remote[512];

typedef struct _src_record {
	int disable;
	uint64_t prefix;
	uint64_t guid;
	uint16_t base_lid;
	uint16_t num_lids;
	uint16_t port_num;
	char hfi_num;
} src_record;

typedef struct _record {
	int disable;
	int simulated;
	uint64_t guid;
	uint16_t lid;
	char name[64];
	int guid_fail_count;
	int lid_fail_count;
} record;


static int 
readline(FILE *f, char *s, int max)
{
	int i, c;

	memset(s,0,max);
	i=0;
	c=fgetc(f);

	while (c !='\n' && !feof(f) && i < max) {
		s[i++]=c;
		c=fgetc(f);
	}
	return i;
}


static int 
readrecord(FILE *f, record *r)
{
	char *p, *l;
	char buffer[1024];

	memset(r, 0, sizeof(record));

	if (readline(f, buffer, 1024) <=0)
		return -1;
			
	r->disable = 0;

	p = strtok_r(buffer, DELIMITER, &l);
	if (!p) 
		return -1;
	r->lid = htons(strtol(p,NULL,0));

	p = strtok_r(NULL, DELIMITER, &l);
	if (!p) 
		return -1;
	r->guid = hton64(strtoull(p,NULL,0));
	
	p = strtok_r(NULL,DELIMITER, &l);
	if (!p) 
		return -1;
	strcpy(r->name, p);
	
	r->simulated = strstr(p,"Sim") != NULL;
	if (!r->simulated) {
		/* parse name */
		p = strstr(r->name," ");
		if (p) 
			*p=0;
	}
	
	return 0;
}

static void 
usage(char **argv)
{
	fprintf(stderr, "Usage: %s [opts] guidtable\n", argv[0]);
	fprintf(stderr, "Test performance of the distributed SA shared memory database.\n");
	fprintf(stderr, "\toptions include:\n");
	fprintf(stderr, "\t--help\n");
	fprintf(stderr, "\t\tProvide this help text.\n");
	fprintf(stderr, "\t-q <queries>\n");
	fprintf(stderr, "\t\tRun at least <queries> queries.\n");
	fprintf(stderr, "\t-p <pkey>\n");
	fprintf(stderr, "\t\tInclude <pkey> in the searches.\n");
	fprintf(stderr,"\t\t(Can be specified up to " add_quotes(MAX_PKEYS)
		       " times.)\n");
	fprintf(stderr, "\t-S <sid>\n");
	fprintf(stderr, "\t\tInclude <sid> in the searches.\n");
	fprintf(stderr,"\t\t(Can be specified up to " add_quotes(MAX_SIDS) 
		       " times. Note that\n");
	fprintf(stderr,"\t\tproviding both SIDs and pkeys may cause problems."
		       ")\n");
	fprintf(stderr, "\n");
	fprintf(stderr,
			"'guidtable' is a text file that lists the destination\n");
	fprintf(stderr,
			"guids and lids (i.e., from build_table.pl)\n");
	fprintf(stderr,
			"\nExample:\t%s -q 100000 -p 0x8001  guidtable\n", argv[0]);
}

static record *dest_ports;
static src_record src_ports[MAX_SOURCE_PORTS];
int numsources = 0;

int
get_sources(void)
{
	umad_port_t port;
	char ca_names[UMAD_MAX_DEVICES][UMAD_CA_NAME_LEN];
	int ca_count;
	int i, err;

	ca_count = umad_get_cas_names(ca_names, UMAD_MAX_DEVICES);

	for (i = 0; i < ca_count; i++) {
		umad_ca_t ca;
		int j;
		
		err = umad_get_ca(ca_names[i], &ca);
		if (err) {
			_DBG_ERROR("Failed to open %s\n", ca_names[i]);
			return -1;
		}

		for (j = 1; j <= ca.numports; j++) {
			err = umad_get_port(ca_names[i], j, &port);
			if (err) {
				_DBG_ERROR("Failed to get info on port %d of"
					   " %s\n", j, ca_names[i]);
				umad_release_ca(&ca);
				return -1;
			}

			if (port.state == IBV_PORT_ACTIVE) {
				src_ports[numsources].disable = 
					(port.state != IBV_PORT_ACTIVE);
				src_ports[numsources].prefix = port.gid_prefix;
				src_ports[numsources].guid = port.port_guid;
				src_ports[numsources].base_lid = 
					htons(port.base_lid);
				src_ports[numsources].num_lids = 
					1 << (port.lmc);
				src_ports[numsources].port_num = j;
				src_ports[numsources].hfi_num = i + 1;
				numsources++;
			}
		}
	}
	return numsources;
}

int 
main(int argc, char **argv)
{
	FILE *f;
	int c, err;
	uint16_t port;
	unsigned i, numdests;
	uint64_t sid[MAX_SIDS];
	unsigned pkey[MAX_PKEYS];
	unsigned numpkeys, numsids;
	uint64_t falsepos, falseneg, passes, queries, req_queries, perf;
	struct timeval start_time, end_time, elapsed_time;
	void *context;
	struct ibv_context *hfi;
	struct ibv_device *device;

	setlocale(LC_ALL, "");

	/* Default values. */
	strcpy(remote,"strife");
	port = 1;
	req_queries = 0;
	numdests = 1024;
	numsources = 0;
	numpkeys = 0;
	numsids = 0;

	if (argc > 1){
		if (!strcmp(argv[1], "--help")){
			usage(argv);
			exit(0);
		}
	}

	while ((c = getopt(argc, argv, "d:q:p:S:")) != EOF) {
		switch (c) {
		case 'p':
			if (numpkeys < MAX_PKEYS) {
				pkey[numpkeys++] = htons(
				   strtoul(optarg, NULL, 0));
			} else {
				_DBG_ERROR("Too many pkeys.\n");
				return -1;
			}
			break;
		case 'S':
			if (numsids < MAX_SIDS) {
				sid[numsids++] = hton64(
				   strtoull(optarg, NULL, 0));
			} else {
				_DBG_ERROR("Too many sids.\n");
				return -1;
			}
			break;
		case 'q':
			req_queries = strtoul(optarg, NULL, 0);
			if (req_queries == ULONG_MAX) {
				_DBG_ERROR("Invalid req_queries arg: %s", optarg);
				return -1;
			}
			break;
		default:
			usage(argv);
			return -1;
		}
	}

	if (optind >= argc) {
		usage(argv);
		return -1;
	}

	f=fopen(argv[optind], "r");
	if (f == NULL) {
		fprintf(stderr, "Failed to open guid file (%s)\n",
			argv[optind]);
		return -1; 
	}

	numsources = get_sources();
	if (numsources < 0) {
		fprintf(stderr, "Could not read source port data.\n");
		fclose(f);
		return -1;
	}

	hfi = op_path_find_hfi("",&device);
	if (!hfi || !device) {
		fprintf(stderr,"Could not open HFI.\n");
		fclose(f);
		return -1;
	}

	context = op_path_open(device, port);
	if (!context) {
		fprintf(stderr, "Could not open path interface.\n");
		goto failopen;
	}

	dest_ports = malloc(sizeof(record) * numdests);
	if (!dest_ports) {
		fprintf(stderr, "Could not allocate memory for destinations.\n");
		goto failmalloc;
	}

	i=0;
	while (!readrecord(f, &dest_ports[i])) {
		i++;
		if (i >= numdests) {
			numdests = numdests * 2;
			dest_ports = realloc(dest_ports, numdests * sizeof(record));
			if (!dest_ports) {
				fprintf(stderr, "Could not allocate memory for destinations.\n");
				goto failmalloc;
			}
		}
	}
		
	numdests = i;
	_DBG_NOTICE("Read %u destinations and %u sources.\n",
			numdests, numsources);
	if (numsources == 0 || numdests == 0) goto fail;

	falsepos = falseneg = queries = passes = 0;
	
	gettimeofday(&start_time, NULL);
	
	srand48(start_time.tv_sec);

	do {
		unsigned j;

		for (j=0; j<numdests; j++) {
			op_path_rec_t query, response;
			int d = j; //lrand48() % numdests;
			int s = lrand48() % numsources;
			int lid_query = 0;

			memset(&query,0,sizeof(query));

			lid_query = lrand48() % 2;
			if (lid_query) {
				unsigned lmc_offset = 
					lrand48()%src_ports[s].num_lids;
				query.slid = htons(
				   ntohs(src_ports[s].base_lid) + lmc_offset);
				query.dlid = htons(
				   ntohs(dest_ports[d].lid) + lmc_offset);
			} else {
				query.sgid.unicast.prefix = 
					src_ports[s].prefix;
				query.sgid.unicast.interface_id = 
					src_ports[s].guid;
				query.dgid.unicast.prefix = 
					src_ports[s].prefix;
				query.dgid.unicast.interface_id = 
					dest_ports[d].guid;
			}
			if (numpkeys) query.pkey = 
				pkey[lrand48() % numpkeys];
			if (numsids) query.service_id = 
				sid[lrand48() % numsids];

			queries++;
			err = op_path_get_path_by_rec(context, &query, 
						      &response);
			if ((err != 0) && (dest_ports[d].disable == 0) &&
				(src_ports[s].disable == 0)) {
					falseneg++;
					if (lid_query) 
						dest_ports[d].lid_fail_count++;
					else
						dest_ports[d].guid_fail_count++;

			} else if ((err == 0) && 
				   ((dest_ports[d].disable != 0) ||
				    (src_ports[s].disable != 0))) {
					falsepos++;
					if (lid_query) 
						dest_ports[d].lid_fail_count++;
					else
						dest_ports[d].guid_fail_count++;
			} else if (lid_query) {
				dest_ports[d].lid_fail_count = 0;
			} else {
				dest_ports[d].guid_fail_count = 0;
			}
		}

		passes++;
	} while (queries < req_queries);

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &start_time, &elapsed_time);
	_DBG_PRINT("%lu queries, %lu false negatives, %lu false positives\n"
		   "%lu passes, elapsed time %ld sec %ld usec.\n",
		   queries, falseneg, falsepos, passes, elapsed_time.tv_sec,
		   elapsed_time.tv_usec);
	if (elapsed_time.tv_sec > 1) {
		perf = queries * 1000 /  (elapsed_time.tv_sec * 1000 + 
					  elapsed_time.tv_usec /1000);
		_DBG_PRINT("Perf: %lu queries/sec.\n", perf);
	}

fail:
	free(dest_ports);
failmalloc:
	op_path_close(context);
failopen:
	ibv_close_device(hfi);
	fclose(f);
	return 0;
}
